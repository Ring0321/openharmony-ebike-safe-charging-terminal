#include "buzzer_control.h"

#include <stdio.h>

#include "iot_pwm.h"

#define SAFE_CHARGE_BUZZER_PWM EPWMDEV_PWM5_M0
#define SAFE_CHARGE_BUZZER_DUTY 20
#define SAFE_CHARGE_BUZZER_FREQ 1000

static bool g_buzzer_state = false;

void buzzer_control_init(void)
{
    unsigned int ret = IoTPwmInit(SAFE_CHARGE_BUZZER_PWM);

    if (ret != 0) {
        printf("[safe_charge] buzzer PWM init failed: %u\n", ret);
    }

    IoTPwmStop(SAFE_CHARGE_BUZZER_PWM);
    g_buzzer_state = false;
}

void buzzer_set_state(bool state)
{
    unsigned int ret = 0;

    if (state == g_buzzer_state) {
        return;
    }

    if (state) {
        ret = IoTPwmStart(SAFE_CHARGE_BUZZER_PWM,
            SAFE_CHARGE_BUZZER_DUTY,
            SAFE_CHARGE_BUZZER_FREQ);
        if (ret != 0) {
            printf("[safe_charge] buzzer start failed: %u\n", ret);
            return;
        }
    } else {
        ret = IoTPwmStop(SAFE_CHARGE_BUZZER_PWM);
        if (ret != 0) {
            printf("[safe_charge] buzzer stop failed: %u\n", ret);
        }
    }

    g_buzzer_state = state;
}

bool buzzer_get_state(void)
{
    return g_buzzer_state;
}
