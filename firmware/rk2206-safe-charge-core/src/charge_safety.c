#include "charge_safety.h"

#include <stdio.h>
#include <string.h>

#include "relay_control.h"

#define SAFE_CHARGE_DEVICE_ID "rk2206_charge_01"
#define SAFE_CHARGE_PORT_ID "port_1"
#define SAFE_CHARGE_USER_ID "guest"
#define SAFE_CHARGE_SESSION_ID "demo_session"
#define SAFE_CHARGE_TICK_SECONDS 1.0f
#define SAFE_CHARGE_OCCUPIED_HOLD_TICKS 30
#define SAFE_CHARGE_FAULT_LOCK_COUNT 3
#define SAFE_CHARGE_TRACE_EVENT_COUNT 4
#define SAFE_CHARGE_RISK_L1_SCORE 35
#define SAFE_CHARGE_RISK_L2_SCORE 65
#define SAFE_CHARGE_RISK_L3_SCORE 85
#define SAFE_CHARGE_FAULT_TEMP 0x01
#define SAFE_CHARGE_FAULT_CURRENT 0x02
#define SAFE_CHARGE_FAULT_SMOKE 0x04
#define SAFE_CHARGE_FAULT_REMOTE 0x08
#define SAFE_CHARGE_SIM_SMOKE_WARN_LEVEL 50.0f
#define SAFE_CHARGE_SIM_TEMP_WARN_OFFSET 2.0f
#define SAFE_CHARGE_SIM_CURRENT_WARN_OFFSET 0.15f
#define SAFE_CHARGE_TEMP_HYSTERESIS 2.0f

static charge_safety_data_t g_charge_data;
static charge_thresholds_t g_thresholds = {
    .temperature_limit = 45.0f,
    .temperature_cutoff = 55.0f,
    .current_limit = 0.80f,
    .current_cutoff = 1.20f,
    .smoke_limit = 35.0f,
    .smoke_cutoff = 70.0f,
    .full_current = 0.08f,
    .full_hold_ticks = 30,
};
static uint16_t g_full_ticks = 0;
static uint16_t g_occupied_ticks = 0;
static uint16_t g_charge_ticks = 0;
static uint8_t g_protect_count = 0;
static uint32_t g_session_no = 1;
static bool g_smoke_simulated = false;
static bool g_gas_warn_simulated = false;
static bool g_temp_warn_simulated = false;
static bool g_current_warn_simulated = false;
static bool g_force_full = false;
static charge_event_t g_recent_events[SAFE_CHARGE_TRACE_EVENT_COUNT];
static uint8_t g_recent_event_count = 0;

static uint8_t charge_max_u8(uint8_t a, uint8_t b)
{
    return (a > b) ? a : b;
}

static uint8_t charge_score_from_limit(float value, float limit, float cutoff)
{
    float remind = limit * 0.8f;
    float score = 0.0f;

    if (cutoff <= limit || limit <= 0.0f) {
        return 0;
    }
    if (value <= 0.0f) {
        return 0;
    }
    if (value >= cutoff) {
        return 100;
    }
    if (value >= limit) {
        score = 65.0f + ((value - limit) * 34.0f / (cutoff - limit));
        return (uint8_t)score;
    }
    if (value >= remind) {
        score = 35.0f + ((value - remind) * 29.0f / (limit - remind));
        return (uint8_t)score;
    }
    score = value * 34.0f / remind;
    return (uint8_t)score;
}

static uint8_t charge_score_from_duration(void)
{
    if (g_charge_ticks >= 720) {
        return 70;
    }
    if (g_charge_ticks >= 360) {
        return 45;
    }
    return 0;
}

static void charge_update_event_trace(charge_event_t event)
{
    char trace[SAFE_CHARGE_EVENT_TRACE_LEN] = {0};
    uint8_t i;

    if (g_recent_event_count < SAFE_CHARGE_TRACE_EVENT_COUNT) {
        g_recent_events[g_recent_event_count++] = event;
    } else {
        for (i = 1; i < SAFE_CHARGE_TRACE_EVENT_COUNT; i++) {
            g_recent_events[i - 1] = g_recent_events[i];
        }
        g_recent_events[SAFE_CHARGE_TRACE_EVENT_COUNT - 1] = event;
    }

    for (i = 0; i < g_recent_event_count; i++) {
        if (i > 0) {
            strncat(trace, ">", sizeof(trace) - strlen(trace) - 1);
        }
        strncat(trace, charge_event_to_string(g_recent_events[i]),
            sizeof(trace) - strlen(trace) - 1);
    }
    strncpy(g_charge_data.event_trace, trace, sizeof(g_charge_data.event_trace) - 1);
}

static void charge_record_event(charge_event_t event)
{
    if (event == CHARGE_EVENT_NONE) {
        return;
    }
    g_charge_data.last_event = event;
    g_charge_data.event_count++;
    charge_update_event_trace(event);
    printf("[safe_charge] event=%s state=%s risk=%s fault=0x%lx\n",
        charge_event_to_string(event),
        charge_state_to_string(g_charge_data.charge_state),
        risk_level_to_string(g_charge_data.risk_level),
        (unsigned long)g_charge_data.fault_code);
}

static void charge_apply_outputs(bool relay_on, bool alarm_on)
{
    relay_set_state(relay_on);
    g_charge_data.relay_state = relay_get_state();
    g_charge_data.alarm_state = alarm_on;
}

static void charge_enter_state(charge_state_t state, charge_event_t event)
{
    charge_state_t old_state = g_charge_data.charge_state;

    if (g_charge_data.charge_state == state && event == CHARGE_EVENT_NONE) {
        return;
    }

    g_charge_data.charge_state = state;

    switch (state) {
        case CHARGE_STATE_IDLE:
            g_charge_data.access_authorized = false;
            g_charge_data.risk_level = RISK_LEVEL_OK;
            g_charge_data.risk_score = 0;
            g_charge_data.fault_code = 0;
            g_full_ticks = 0;
            g_occupied_ticks = 0;
            g_charge_ticks = 0;
            charge_apply_outputs(false, false);
            break;
        case CHARGE_STATE_FULL:
            g_charge_data.access_authorized = false;
            g_charge_data.risk_level = RISK_LEVEL_OK;
            g_charge_data.risk_score = 0;
            g_charge_data.fault_code = 0;
            g_occupied_ticks = 0;
            charge_apply_outputs(false, false);
            break;
        case CHARGE_STATE_OCCUPIED:
            g_charge_data.access_authorized = false;
            g_charge_data.risk_level = RISK_LEVEL_L1_REMIND;
            if (g_charge_data.risk_score < SAFE_CHARGE_RISK_L1_SCORE) {
                g_charge_data.risk_score = SAFE_CHARGE_RISK_L1_SCORE;
            }
            charge_apply_outputs(false, true);
            break;
        case CHARGE_STATE_CHARGING:
            if (old_state != CHARGE_STATE_ALERT) {
                g_charge_data.risk_level = RISK_LEVEL_OK;
                g_charge_data.risk_score = 0;
                g_charge_data.fault_code = 0;
                g_full_ticks = 0;
                g_occupied_ticks = 0;
            }
            charge_apply_outputs(true, false);
            break;
        case CHARGE_STATE_ALERT:
            if (g_charge_data.risk_level < RISK_LEVEL_L2_WARN) {
                g_charge_data.risk_level = RISK_LEVEL_L2_WARN;
            }
            charge_apply_outputs(true, true);
            break;
        case CHARGE_STATE_CUT_OFF:
            g_charge_data.access_authorized = false;
            g_charge_data.risk_level = RISK_LEVEL_L3_PROTECT;
            if (g_charge_data.risk_score < SAFE_CHARGE_RISK_L3_SCORE) {
                g_charge_data.risk_score = SAFE_CHARGE_RISK_L3_SCORE;
            }
            charge_apply_outputs(false, true);
            break;
        case CHARGE_STATE_FAULT:
            g_charge_data.access_authorized = false;
            g_charge_data.risk_level = RISK_LEVEL_L4_FAULT;
            g_charge_data.risk_score = 100;
            charge_apply_outputs(false, true);
            break;
        default:
            break;
    }

    charge_record_event(event);
}

static void charge_clear_demo_warnings(void)
{
    g_smoke_simulated = false;
    g_gas_warn_simulated = false;
    g_temp_warn_simulated = false;
    g_current_warn_simulated = false;
}

void charge_safety_init(void)
{
    memset(&g_charge_data, 0, sizeof(g_charge_data));
    strcpy(g_charge_data.device_id, SAFE_CHARGE_DEVICE_ID);
    strcpy(g_charge_data.port_id, SAFE_CHARGE_PORT_ID);
    strcpy(g_charge_data.user_id, SAFE_CHARGE_USER_ID);
    strcpy(g_charge_data.session_id, SAFE_CHARGE_SESSION_ID);

    relay_control_init();
    power_sensor_init();
    charge_enter_state(CHARGE_STATE_IDLE, CHARGE_EVENT_NONE);
}

static void charge_update_risk_score(void)
{
    uint8_t score = 0;

    score = charge_max_u8(score, charge_score_from_limit(
        g_charge_data.temperature_c,
        g_thresholds.temperature_limit,
        g_thresholds.temperature_cutoff));
    score = charge_max_u8(score, charge_score_from_limit(
        g_charge_data.power.current_a,
        g_thresholds.current_limit,
        g_thresholds.current_cutoff));
    score = charge_max_u8(score, charge_score_from_limit(
        g_charge_data.smoke_level,
        g_thresholds.smoke_limit,
        g_thresholds.smoke_cutoff));
    score = charge_max_u8(score, charge_score_from_duration());

    g_charge_data.risk_score = score;
}

static void charge_evaluate_risk(void)
{
    bool should_cutoff = false;
    bool should_warn = false;
    float temperature_warn_limit = g_thresholds.temperature_limit;
    charge_event_t event = CHARGE_EVENT_NONE;

    g_charge_data.risk_level = RISK_LEVEL_OK;
    g_charge_data.fault_code = 0;
    charge_update_risk_score();

    if (g_charge_data.charge_state == CHARGE_STATE_ALERT &&
        g_charge_data.last_event == CHARGE_EVENT_TEMP_WARN &&
        temperature_warn_limit > SAFE_CHARGE_TEMP_HYSTERESIS) {
        temperature_warn_limit -= SAFE_CHARGE_TEMP_HYSTERESIS;
    }

    if (g_charge_data.temperature_c >= g_thresholds.temperature_cutoff) {
        should_cutoff = true;
        g_charge_data.fault_code |= SAFE_CHARGE_FAULT_TEMP;
        event = CHARGE_EVENT_PROTECT_CUTOFF;
    } else if (g_charge_data.temperature_c >= temperature_warn_limit) {
        should_warn = true;
        event = CHARGE_EVENT_TEMP_WARN;
    }

    if (g_charge_data.power.current_a >= g_thresholds.current_cutoff) {
        should_cutoff = true;
        g_charge_data.fault_code |= SAFE_CHARGE_FAULT_CURRENT;
        event = CHARGE_EVENT_PROTECT_CUTOFF;
    } else if (g_charge_data.power.current_a >= g_thresholds.current_limit) {
        should_warn = true;
        if (event == CHARGE_EVENT_NONE) {
            event = CHARGE_EVENT_CURRENT_WARN;
        }
    }

    if (g_charge_data.smoke_level >= g_thresholds.smoke_cutoff) {
        should_cutoff = true;
        g_charge_data.fault_code |= SAFE_CHARGE_FAULT_SMOKE;
        event = CHARGE_EVENT_PROTECT_CUTOFF;
    } else if (g_charge_data.smoke_level >= g_thresholds.smoke_limit) {
        should_warn = true;
        if (event == CHARGE_EVENT_NONE) {
            event = g_gas_warn_simulated ? CHARGE_EVENT_GAS_WARN : CHARGE_EVENT_SMOKE_WARN;
        }
    }

    if (should_cutoff) {
        g_protect_count++;
        if (g_protect_count >= SAFE_CHARGE_FAULT_LOCK_COUNT) {
            g_charge_data.risk_level = RISK_LEVEL_L4_FAULT;
            charge_enter_state(CHARGE_STATE_FAULT, CHARGE_EVENT_LOCKED);
        } else {
            g_charge_data.risk_level = RISK_LEVEL_L3_PROTECT;
            charge_enter_state(CHARGE_STATE_CUT_OFF, event);
        }
        return;
    }

    if (should_warn || g_charge_data.risk_score >= SAFE_CHARGE_RISK_L2_SCORE) {
        g_charge_data.risk_level = RISK_LEVEL_L2_WARN;
        if (g_charge_data.charge_state == CHARGE_STATE_ALERT &&
            g_charge_data.last_event == event) {
            event = CHARGE_EVENT_NONE;
        }
        charge_enter_state(CHARGE_STATE_ALERT, event);
        return;
    }

    if (g_charge_data.risk_score >= SAFE_CHARGE_RISK_L1_SCORE) {
        g_charge_data.risk_level = RISK_LEVEL_L1_REMIND;
        if (g_charge_data.charge_state == CHARGE_STATE_ALERT) {
            charge_enter_state(CHARGE_STATE_CHARGING, CHARGE_EVENT_NONE);
        }
        return;
    }

    if (g_charge_data.charge_state == CHARGE_STATE_ALERT) {
        charge_enter_state(CHARGE_STATE_CHARGING, CHARGE_EVENT_NONE);
    }
}

void charge_safety_update(double temperature, double humidity, double illumination,
    double smoke_ppm, bool human_present)
{
    bool load_enabled =
        (g_charge_data.charge_state == CHARGE_STATE_CHARGING ||
         g_charge_data.charge_state == CHARGE_STATE_ALERT);

    g_charge_data.temperature_c = (float)temperature;
    g_charge_data.humidity_pct = (float)humidity;
    g_charge_data.illumination_lx = (float)illumination;
    g_charge_data.human_present = human_present;
    if (g_temp_warn_simulated) {
        g_charge_data.temperature_c = g_thresholds.temperature_limit +
            SAFE_CHARGE_SIM_TEMP_WARN_OFFSET;
    }
    g_charge_data.smoke_level = (float)smoke_ppm;
    if (g_charge_data.smoke_level < 0.0f) {
        g_charge_data.smoke_level = 0.0f;
    }
    if (g_smoke_simulated || g_gas_warn_simulated) {
        g_charge_data.smoke_level = SAFE_CHARGE_SIM_SMOKE_WARN_LEVEL;
    }

    if (g_charge_data.charge_state == CHARGE_STATE_FULL) {
        g_occupied_ticks++;
        if (g_occupied_ticks >= SAFE_CHARGE_OCCUPIED_HOLD_TICKS) {
            charge_enter_state(CHARGE_STATE_OCCUPIED, CHARGE_EVENT_OCCUPIED);
        }
        return;
    }

    power_sensor_read(&g_charge_data.power, load_enabled);
    g_charge_data.sensor_simulated = g_charge_data.power.simulated;
    if (load_enabled && g_current_warn_simulated) {
        g_charge_data.power.current_a = g_thresholds.current_limit +
            SAFE_CHARGE_SIM_CURRENT_WARN_OFFSET;
        g_charge_data.power.power_w = g_charge_data.power.voltage_v *
            g_charge_data.power.current_a;
    }

    if (load_enabled) {
        g_charge_ticks++;
        g_charge_data.energy_wh += g_charge_data.power.power_w *
            (SAFE_CHARGE_TICK_SECONDS / 3600.0f);
        g_charge_data.cost_estimate = g_charge_data.energy_wh * 0.001f;
    }

    if (g_force_full) {
        charge_enter_state(CHARGE_STATE_FULL, CHARGE_EVENT_FULL);
        g_force_full = false;
        return;
    }

    if (load_enabled && g_charge_data.power.current_a < g_thresholds.full_current) {
        g_full_ticks++;
        if (g_full_ticks >= g_thresholds.full_hold_ticks) {
            charge_enter_state(CHARGE_STATE_FULL, CHARGE_EVENT_FULL);
            return;
        }
    } else {
        g_full_ticks = 0;
    }

    if (load_enabled) {
        charge_evaluate_risk();
    }
}

void charge_safety_handle_command(charge_command_t cmd)
{
    switch (cmd) {
        case CHARGE_CMD_START:
            if (!g_charge_data.access_authorized) {
                charge_record_event(CHARGE_EVENT_AUTH_DENY);
                charge_enter_state(CHARGE_STATE_IDLE, CHARGE_EVENT_NONE);
                break;
            }
            relay_force_cutoff(false);
            charge_clear_demo_warnings();
            g_force_full = false;
            g_full_ticks = 0;
            g_occupied_ticks = 0;
            g_charge_ticks = 0;
            g_charge_data.energy_wh = 0.0f;
            g_charge_data.cost_estimate = 0.0f;
            snprintf(g_charge_data.session_id, sizeof(g_charge_data.session_id),
                "demo_%lu", (unsigned long)g_session_no++);
            charge_enter_state(CHARGE_STATE_CHARGING, CHARGE_EVENT_START);
            break;
        case CHARGE_CMD_STOP:
            relay_force_cutoff(false);
            g_charge_data.access_authorized = false;
            charge_enter_state(CHARGE_STATE_IDLE, CHARGE_EVENT_STOP);
            break;
        case CHARGE_CMD_RESET:
            relay_force_cutoff(false);
            g_charge_data.access_authorized = false;
            charge_clear_demo_warnings();
            g_force_full = false;
            g_full_ticks = 0;
            g_occupied_ticks = 0;
            g_charge_ticks = 0;
            if (g_charge_data.charge_state == CHARGE_STATE_FAULT) {
                g_protect_count = 0;
            }
            charge_enter_state(CHARGE_STATE_IDLE, CHARGE_EVENT_MANUAL_RESET);
            break;
        case CHARGE_CMD_RELAY_OFF:
            relay_force_cutoff(true);
            g_charge_data.access_authorized = false;
            g_charge_data.fault_code |= SAFE_CHARGE_FAULT_REMOTE;
            g_charge_data.risk_level = RISK_LEVEL_L3_PROTECT;
            charge_enter_state(CHARGE_STATE_CUT_OFF, CHARGE_EVENT_REMOTE_CUTOFF);
            break;
        case CHARGE_CMD_SIM_SMOKE:
            g_smoke_simulated = true;
            charge_record_event(CHARGE_EVENT_SMOKE_WARN);
            break;
        case CHARGE_CMD_CLEAR_SMOKE:
            g_smoke_simulated = false;
            g_gas_warn_simulated = false;
            break;
        case CHARGE_CMD_SIM_FULL:
            g_force_full = true;
            break;
        case CHARGE_CMD_SIM_TEMP_WARN:
            g_temp_warn_simulated = true;
            charge_record_event(CHARGE_EVENT_TEMP_WARN);
            break;
        case CHARGE_CMD_SIM_CURRENT_WARN:
            g_current_warn_simulated = true;
            charge_record_event(CHARGE_EVENT_CURRENT_WARN);
            break;
        case CHARGE_CMD_SIM_GAS_WARN:
            g_gas_warn_simulated = true;
            g_charge_data.smoke_level = SAFE_CHARGE_SIM_SMOKE_WARN_LEVEL;
            if (g_charge_data.risk_score < SAFE_CHARGE_RISK_L2_SCORE) {
                g_charge_data.risk_score = SAFE_CHARGE_RISK_L2_SCORE;
            }
            g_charge_data.risk_level = RISK_LEVEL_L2_WARN;
            if (g_charge_data.charge_state == CHARGE_STATE_CHARGING ||
                g_charge_data.charge_state == CHARGE_STATE_ALERT) {
                charge_enter_state(CHARGE_STATE_ALERT, CHARGE_EVENT_GAS_WARN);
            } else {
                charge_record_event(CHARGE_EVENT_GAS_WARN);
            }
            break;
        case CHARGE_CMD_DEMO_NEXT:
            if (g_charge_data.charge_state == CHARGE_STATE_IDLE) {
                charge_safety_handle_command(CHARGE_CMD_ACCESS_AUTH_OK);
            } else if (g_charge_data.charge_state == CHARGE_STATE_CHARGING) {
                charge_safety_handle_command(CHARGE_CMD_SIM_GAS_WARN);
            } else if (g_charge_data.charge_state == CHARGE_STATE_ALERT) {
                charge_safety_handle_command(CHARGE_CMD_RELAY_OFF);
            } else {
                charge_safety_handle_command(CHARGE_CMD_RESET);
            }
            break;
        case CHARGE_CMD_ACCESS_AUTH_OK:
            if (g_charge_data.charge_state == CHARGE_STATE_CUT_OFF ||
                g_charge_data.charge_state == CHARGE_STATE_FAULT ||
                relay_is_force_cutoff()) {
                charge_record_event(CHARGE_EVENT_AUTH_DENY);
                break;
            }
            if (g_charge_data.charge_state != CHARGE_STATE_IDLE) {
                charge_record_event(CHARGE_EVENT_AUTH_OK);
                break;
            }
            g_charge_data.access_authorized = true;
            charge_record_event(CHARGE_EVENT_AUTH_OK);
            charge_safety_handle_command(CHARGE_CMD_START);
            break;
        case CHARGE_CMD_ACCESS_AUTH_DENY:
            g_charge_data.access_authorized = false;
            charge_record_event(CHARGE_EVENT_AUTH_DENY);
            if (g_charge_data.charge_state == CHARGE_STATE_CHARGING ||
                g_charge_data.charge_state == CHARGE_STATE_ALERT) {
                relay_force_cutoff(true);
                g_charge_data.fault_code |= SAFE_CHARGE_FAULT_REMOTE;
                charge_enter_state(CHARGE_STATE_CUT_OFF, CHARGE_EVENT_REMOTE_CUTOFF);
            } else {
                charge_enter_state(CHARGE_STATE_IDLE, CHARGE_EVENT_NONE);
            }
            break;
        default:
            break;
    }
}

void charge_safety_set_thresholds(const charge_thresholds_t *thresholds)
{
    if (thresholds == NULL) {
        return;
    }

    g_thresholds = *thresholds;
    if (g_thresholds.temperature_cutoff <= g_thresholds.temperature_limit) {
        g_thresholds.temperature_cutoff = g_thresholds.temperature_limit + 5.0f;
    }
    if (g_thresholds.current_cutoff <= g_thresholds.current_limit) {
        g_thresholds.current_cutoff = g_thresholds.current_limit + 0.2f;
    }
    if (g_thresholds.smoke_cutoff <= g_thresholds.smoke_limit) {
        g_thresholds.smoke_cutoff = g_thresholds.smoke_limit + 10.0f;
    }
    if (g_thresholds.full_hold_ticks == 0) {
        g_thresholds.full_hold_ticks = 1;
    }
    charge_record_event(CHARGE_EVENT_THRESHOLD_UPDATE);
}

void charge_safety_get_thresholds(charge_thresholds_t *thresholds)
{
    if (thresholds == NULL) {
        return;
    }
    *thresholds = g_thresholds;
}

void charge_safety_set_network_state(bool state)
{
    g_charge_data.network_state = state;
}

void charge_safety_get_data(charge_safety_data_t *data)
{
    if (data == NULL) {
        return;
    }
    *data = g_charge_data;
}

const char *charge_state_to_string(charge_state_t state)
{
    switch (state) {
        case CHARGE_STATE_IDLE: return "IDLE";
        case CHARGE_STATE_CHARGING: return "CHARGING";
        case CHARGE_STATE_FULL: return "FULL";
        case CHARGE_STATE_OCCUPIED: return "OCCUPIED";
        case CHARGE_STATE_ALERT: return "ALERT";
        case CHARGE_STATE_CUT_OFF: return "CUTOFF";
        case CHARGE_STATE_FAULT: return "LOCKED";
        default: return "UNKNOWN";
    }
}

const char *risk_level_to_string(risk_level_t level)
{
    switch (level) {
        case RISK_LEVEL_OK: return "OK";
        case RISK_LEVEL_L1_REMIND: return "L1_NOTICE";
        case RISK_LEVEL_L2_WARN: return "L2_ALARM";
        case RISK_LEVEL_L3_PROTECT: return "L3_CUTOFF";
        case RISK_LEVEL_L4_FAULT: return "L4_LOCKED";
        default: return "UNKNOWN";
    }
}

const char *charge_event_to_string(charge_event_t event)
{
    switch (event) {
        case CHARGE_EVENT_NONE: return "NONE";
        case CHARGE_EVENT_START: return "START";
        case CHARGE_EVENT_STOP: return "STOP";
        case CHARGE_EVENT_FULL: return "FULL";
        case CHARGE_EVENT_OCCUPIED: return "OCCUPIED";
        case CHARGE_EVENT_TEMP_WARN: return "TEMP_WARN";
        case CHARGE_EVENT_CURRENT_WARN: return "CURRENT_WARN";
        case CHARGE_EVENT_SMOKE_WARN: return "SMOKE_WARN";
        case CHARGE_EVENT_GAS_WARN: return "GAS_WARN";
        case CHARGE_EVENT_PROTECT_CUTOFF: return "PROTECT_CUTOFF";
        case CHARGE_EVENT_MANUAL_RESET: return "MANUAL_RESET";
        case CHARGE_EVENT_REMOTE_CUTOFF: return "REMOTE_CUTOFF";
        case CHARGE_EVENT_LOCKED: return "LOCKED";
        case CHARGE_EVENT_THRESHOLD_UPDATE: return "THRESHOLD_UPDATE";
        case CHARGE_EVENT_AUTH_OK: return "AUTH_OK";
        case CHARGE_EVENT_AUTH_DENY: return "AUTH_DENY";
        default: return "UNKNOWN";
    }
}
