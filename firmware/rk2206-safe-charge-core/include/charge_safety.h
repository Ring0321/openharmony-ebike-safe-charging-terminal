#ifndef CHARGE_SAFETY_H
#define CHARGE_SAFETY_H

#include <stdbool.h>
#include <stdint.h>

#include "power_sensor.h"

#define SAFE_CHARGE_EVENT_TRACE_LEN 96

typedef enum {
    CHARGE_STATE_IDLE = 0,
    CHARGE_STATE_CHARGING,
    CHARGE_STATE_FULL,
    CHARGE_STATE_OCCUPIED,
    CHARGE_STATE_ALERT,
    CHARGE_STATE_CUT_OFF,
    CHARGE_STATE_FAULT,
} charge_state_t;

typedef enum {
    RISK_LEVEL_OK = 0,
    RISK_LEVEL_L1_REMIND,
    RISK_LEVEL_L2_WARN,
    RISK_LEVEL_L3_PROTECT,
    RISK_LEVEL_L4_FAULT,
} risk_level_t;

typedef enum {
    CHARGE_EVENT_NONE = 0,
    CHARGE_EVENT_START,
    CHARGE_EVENT_STOP,
    CHARGE_EVENT_FULL,
    CHARGE_EVENT_OCCUPIED,
    CHARGE_EVENT_TEMP_WARN,
    CHARGE_EVENT_CURRENT_WARN,
    CHARGE_EVENT_SMOKE_WARN,
    CHARGE_EVENT_GAS_WARN,
    CHARGE_EVENT_PROTECT_CUTOFF,
    CHARGE_EVENT_MANUAL_RESET,
    CHARGE_EVENT_REMOTE_CUTOFF,
    CHARGE_EVENT_LOCKED,
    CHARGE_EVENT_THRESHOLD_UPDATE,
    CHARGE_EVENT_AUTH_OK,
    CHARGE_EVENT_AUTH_DENY,
} charge_event_t;

typedef struct {
    float temperature_limit;
    float temperature_cutoff;
    float current_limit;
    float current_cutoff;
    float smoke_limit;
    float smoke_cutoff;
    float full_current;
    uint16_t full_hold_ticks;
} charge_thresholds_t;

typedef struct {
    char device_id[24];
    char port_id[16];
    char user_id[24];
    char session_id[24];
    charge_state_t charge_state;
    risk_level_t risk_level;
    uint8_t risk_score;
    charge_event_t last_event;
    power_sensor_data_t power;
    float temperature_c;
    float humidity_pct;
    float illumination_lx;
    float smoke_level;
    bool human_present;
    float energy_wh;
    float cost_estimate;
    bool relay_state;
    bool alarm_state;
    bool network_state;
    bool access_authorized;
    bool sensor_simulated;
    uint32_t event_count;
    uint32_t fault_code;
    char event_trace[SAFE_CHARGE_EVENT_TRACE_LEN];
} charge_safety_data_t;

typedef enum {
    CHARGE_CMD_START = 1,
    CHARGE_CMD_STOP,
    CHARGE_CMD_RESET,
    CHARGE_CMD_RELAY_OFF,
    CHARGE_CMD_SIM_SMOKE,
    CHARGE_CMD_CLEAR_SMOKE,
    CHARGE_CMD_SIM_FULL,
    CHARGE_CMD_SIM_TEMP_WARN,
    CHARGE_CMD_SIM_CURRENT_WARN,
    CHARGE_CMD_SIM_GAS_WARN,
    CHARGE_CMD_DEMO_NEXT,
    CHARGE_CMD_ACCESS_AUTH_OK,
    CHARGE_CMD_ACCESS_AUTH_DENY,
} charge_command_t;

void charge_safety_init(void);
void charge_safety_update(double temperature, double humidity, double illumination,
    double smoke_ppm, bool human_present);
void charge_safety_handle_command(charge_command_t cmd);
void charge_safety_set_thresholds(const charge_thresholds_t *thresholds);
void charge_safety_get_thresholds(charge_thresholds_t *thresholds);
void charge_safety_set_network_state(bool state);
void charge_safety_get_data(charge_safety_data_t *data);
const char *charge_state_to_string(charge_state_t state);
const char *risk_level_to_string(risk_level_t level);
const char *charge_event_to_string(charge_event_t event);

#endif
