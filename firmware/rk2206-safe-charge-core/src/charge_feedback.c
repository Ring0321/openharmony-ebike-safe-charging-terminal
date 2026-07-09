#include "charge_feedback.h"

#include <stdio.h>

#include "buzzer_control.h"
#include "drv_light.h"
#include "drv_motor.h"
#include "relay_control.h"
#include "su_03t.h"

#ifndef SAFE_CHARGE_BOARD_MOTOR_ENABLE
#define SAFE_CHARGE_BOARD_MOTOR_ENABLE 1
#endif

static bool g_motor_active = false;
static bool g_cooling_active = false;
static bool g_warning_relay_active = false;
static bool g_protection_relay_active = false;
static bool g_alarm_active = false;
static light_color_t g_feedback_color = LIGHT_COLOR_OFF;
static charge_state_t g_last_state = CHARGE_STATE_IDLE;
static uint32_t g_last_voice_event_count = 0;
static uint8_t g_blink_tick = 0;

static void feedback_set_motor(bool enabled)
{
#if SAFE_CHARGE_BOARD_MOTOR_ENABLE
    if (enabled == g_motor_active) {
        return;
    }
    motor_set_state(enabled);
    g_motor_active = enabled;
#else
    (void)enabled;
    motor_set_state(false);
    g_motor_active = false;
#endif
}

static void feedback_set_cooling(bool enabled)
{
    if (enabled == g_cooling_active) {
        return;
    }
    relay_set_cooling_state(enabled);
    g_cooling_active = enabled;
}

static void feedback_set_warning_relay(bool enabled)
{
    if (enabled == g_warning_relay_active) {
        return;
    }
    relay_set_warning_state(enabled);
    g_warning_relay_active = enabled;
}

static void feedback_set_protection_relay(bool enabled)
{
    if (enabled == g_protection_relay_active) {
        return;
    }
    relay_set_protection_state(enabled);
    g_protection_relay_active = enabled;
}

static void feedback_set_light(light_color_t color)
{
    if (color == g_feedback_color) {
        return;
    }
    light_set_color(color);
    g_feedback_color = color;
}

static bool feedback_is_voice_event(charge_event_t event)
{
    switch (event) {
        case CHARGE_EVENT_START:
        case CHARGE_EVENT_FULL:
        case CHARGE_EVENT_OCCUPIED:
        case CHARGE_EVENT_TEMP_WARN:
        case CHARGE_EVENT_CURRENT_WARN:
        case CHARGE_EVENT_SMOKE_WARN:
        case CHARGE_EVENT_GAS_WARN:
        case CHARGE_EVENT_PROTECT_CUTOFF:
        case CHARGE_EVENT_MANUAL_RESET:
        case CHARGE_EVENT_REMOTE_CUTOFF:
        case CHARGE_EVENT_LOCKED:
            return true;
        default:
            return false;
    }
}

static void feedback_play_voice(const charge_safety_data_t *data, charge_state_t old_state)
{
    if (data->charge_state == CHARGE_STATE_IDLE && old_state != CHARGE_STATE_IDLE) {
        su03t_play_event(SU03T_EVENT_RESET);
        return;
    }
    su03t_play_event_by_charge_data(data);
}

void charge_feedback_init(void)
{
    buzzer_control_init();
    g_motor_active = false;
    g_cooling_active = false;
    g_warning_relay_active = false;
    g_protection_relay_active = false;
    g_alarm_active = false;
    g_feedback_color = LIGHT_COLOR_OFF;
    g_last_state = CHARGE_STATE_IDLE;
    g_last_voice_event_count = 0;
    g_blink_tick = 0;
    feedback_set_motor(false);
    feedback_set_cooling(false);
    feedback_set_warning_relay(false);
    feedback_set_protection_relay(false);
    feedback_set_light(LIGHT_COLOR_BLUE);
    buzzer_set_state(false);
}

void charge_feedback_emergency_cutoff(void)
{
    relay_force_cutoff(true);
    relay_set_state(false);
    motor_set_state(false);
    g_motor_active = false;
    relay_set_cooling_state(true);
    g_cooling_active = true;
    relay_set_warning_state(true);
    g_warning_relay_active = true;
    relay_set_protection_state(true);
    g_protection_relay_active = true;
    feedback_set_light(LIGHT_COLOR_RED);
    buzzer_set_state(true);
    g_alarm_active = true;
}

void charge_feedback_reset_idle(void)
{
    relay_force_cutoff(false);
    relay_set_state(false);
    relay_set_cooling_state(false);
    g_cooling_active = false;
    relay_set_warning_state(false);
    g_warning_relay_active = false;
    relay_set_protection_state(false);
    g_protection_relay_active = false;
    motor_set_state(false);
    g_motor_active = false;
    feedback_set_light(LIGHT_COLOR_BLUE);
    buzzer_set_state(false);
    g_alarm_active = false;
}

void charge_feedback_apply(const charge_safety_data_t *data)
{
    light_color_t color = LIGHT_COLOR_BLUE;
    bool motor = false;
    bool cooling = false;
    bool warning_relay = false;
    bool protection_relay = false;
    bool alarm = false;
    bool state_changed = false;
    bool voice_event_changed = false;
    charge_state_t old_state = g_last_state;

    if (data == NULL) {
        return;
    }

    state_changed = (data->charge_state != g_last_state);
    voice_event_changed = (data->event_count != g_last_voice_event_count &&
        feedback_is_voice_event(data->last_event));

    if (state_changed || voice_event_changed) {
        printf("[safe_charge] feedback state=%s risk=%s\n",
            charge_state_to_string(data->charge_state),
            risk_level_to_string(data->risk_level));
        feedback_play_voice(data, old_state);
        g_last_voice_event_count = data->event_count;
    }

    if (state_changed) {
        g_last_state = data->charge_state;
        g_blink_tick = 0;
    }

    switch (data->charge_state) {
        case CHARGE_STATE_CHARGING:
            color = LIGHT_COLOR_GREEN;
            motor = true;
            cooling = (data->risk_level >= RISK_LEVEL_L1_REMIND);
            break;
        case CHARGE_STATE_FULL:
            color = LIGHT_COLOR_GREEN;
            break;
        case CHARGE_STATE_OCCUPIED:
            color = LIGHT_COLOR_ORANGE;
            warning_relay = true;
            alarm = ((g_blink_tick++ & 1) == 0);
            break;
        case CHARGE_STATE_ALERT:
            g_blink_tick++;
            color = (g_blink_tick & 1) ? LIGHT_COLOR_ORANGE : LIGHT_COLOR_RED;
            motor = true;
            cooling = true;
            warning_relay = true;
            alarm = ((g_blink_tick & 1) != 0);
            break;
        case CHARGE_STATE_CUT_OFF:
            color = LIGHT_COLOR_RED;
            cooling = true;
            warning_relay = true;
            protection_relay = true;
            alarm = true;
            break;
        case CHARGE_STATE_FAULT:
            g_blink_tick++;
            color = (g_blink_tick & 1) ? LIGHT_COLOR_RED : LIGHT_COLOR_OFF;
            cooling = true;
            warning_relay = true;
            protection_relay = true;
            alarm = true;
            break;
        case CHARGE_STATE_IDLE:
        default:
            color = LIGHT_COLOR_BLUE;
            break;
    }

    if (data->temperature_c >= 45.0f &&
        data->charge_state != CHARGE_STATE_FAULT) {
        cooling = true;
    }

    motor = data->relay_state;
    if (relay_is_force_cutoff()) {
        motor = false;
        cooling = true;
        warning_relay = true;
        protection_relay = true;
        alarm = true;
        color = LIGHT_COLOR_RED;
    }

    feedback_set_motor(motor);
    feedback_set_cooling(cooling);
    feedback_set_warning_relay(warning_relay);
    feedback_set_protection_relay(protection_relay);
    feedback_set_light(color);
    buzzer_set_state(alarm);
    g_alarm_active = buzzer_get_state();
}

bool charge_feedback_motor_active(void)
{
    return g_motor_active;
}

bool charge_feedback_cooling_active(void)
{
    return g_cooling_active;
}

bool charge_feedback_alarm_active(void)
{
    return g_alarm_active;
}

const char *charge_feedback_cooling_status(void)
{
    return g_cooling_active ? "ON" : "OFF";
}

const char *charge_feedback_motor_status(void)
{
    return g_motor_active ? "ON" : "OFF";
}

const char *charge_feedback_light_status(void)
{
    return light_color_to_string(g_feedback_color);
}

uint8_t charge_feedback_demo_step(charge_state_t state)
{
    switch (state) {
        case CHARGE_STATE_CHARGING:
            return 2;
        case CHARGE_STATE_FULL:
        case CHARGE_STATE_OCCUPIED:
        case CHARGE_STATE_ALERT:
            return 3;
        case CHARGE_STATE_CUT_OFF:
        case CHARGE_STATE_FAULT:
            return 4;
        case CHARGE_STATE_IDLE:
        default:
            return 1;
    }
}
