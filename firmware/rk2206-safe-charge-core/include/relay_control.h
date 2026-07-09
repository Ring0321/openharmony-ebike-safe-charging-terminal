#ifndef RELAY_CONTROL_H
#define RELAY_CONTROL_H

#include <stdbool.h>

typedef enum {
    SAFE_CHARGE_RELAY_K1_OUTPUT = 0,
    SAFE_CHARGE_RELAY_K2_COOLING,
    SAFE_CHARGE_RELAY_K3_WARNING,
    SAFE_CHARGE_RELAY_K4_PROTECTION,
    SAFE_CHARGE_RELAY_COUNT,
} safe_charge_relay_channel_t;

void relay_control_init(void);
void relay_set_channel_state(safe_charge_relay_channel_t channel, bool state);
bool relay_get_channel_state(safe_charge_relay_channel_t channel);
const char *relay_get_channel_role(safe_charge_relay_channel_t channel);
void relay_set_state(bool state);
bool relay_get_state(void);
void relay_force_cutoff(bool enabled);
bool relay_is_force_cutoff(void);
void relay_set_cooling_state(bool state);
bool relay_get_cooling_state(void);
void relay_set_warning_state(bool state);
bool relay_get_warning_state(void);
void relay_set_protection_state(bool state);
bool relay_get_protection_state(void);
void relay_all_off(void);

#endif
