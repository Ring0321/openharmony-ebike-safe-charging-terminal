#ifndef CHARGE_FEEDBACK_H
#define CHARGE_FEEDBACK_H

#include <stdbool.h>
#include <stdint.h>

#include "charge_safety.h"

void charge_feedback_init(void);
void charge_feedback_apply(const charge_safety_data_t *data);
void charge_feedback_emergency_cutoff(void);
void charge_feedback_reset_idle(void);
bool charge_feedback_motor_active(void);
bool charge_feedback_cooling_active(void);
bool charge_feedback_alarm_active(void);
const char *charge_feedback_motor_status(void);
const char *charge_feedback_cooling_status(void);
const char *charge_feedback_light_status(void);
uint8_t charge_feedback_demo_step(charge_state_t state);

#endif
