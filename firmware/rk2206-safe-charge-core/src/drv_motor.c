#include "drv_motor.h"
#include "iot_pwm.h"

static bool g_motor_state = false;

#define MOTOR_PWM_HANDLE EPWMDEV_PWM6_M0

void motor_dev_init(void)
{
    IoTPwmInit(MOTOR_PWM_HANDLE);
    IoTPwmStop(MOTOR_PWM_HANDLE);
    g_motor_state = false;
}

void motor_set_pwm(unsigned int duty)
{
    IoTPwmStart(MOTOR_PWM_HANDLE, duty, 1000);
}

void motor_set_state(bool state)
{

    if (state == g_motor_state)
    {
        return;
    }

    if (state)
    {
        motor_set_pwm(20);
    }
    else
    {
        motor_set_pwm(1);
        IoTPwmStop(MOTOR_PWM_HANDLE);
    }
    g_motor_state = state;

}

int get_motor_state(void)
{
    return g_motor_state;
}
