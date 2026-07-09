#ifndef BUZZER_CONTROL_H
#define BUZZER_CONTROL_H

#include <stdbool.h>

void buzzer_control_init(void);
void buzzer_set_state(bool state);
bool buzzer_get_state(void);

#endif
