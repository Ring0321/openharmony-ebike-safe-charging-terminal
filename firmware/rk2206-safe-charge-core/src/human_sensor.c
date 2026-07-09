#include "human_sensor.h"

#include <stdio.h>

#include "iot_errno.h"
#include "iot_gpio.h"

#ifndef SAFE_CHARGE_HUMAN_SENSOR_GPIO
#define SAFE_CHARGE_HUMAN_SENSOR_GPIO GPIO0_PA3
#endif

#ifndef SAFE_CHARGE_HUMAN_SENSOR_ACTIVE_HIGH
#define SAFE_CHARGE_HUMAN_SENSOR_ACTIVE_HIGH 1
#endif

static bool g_human_sensor_ready = false;

void human_sensor_init(void)
{
    unsigned int ret;

    ret = IoTGpioInit(SAFE_CHARGE_HUMAN_SENSOR_GPIO);
    if (ret != IOT_SUCCESS) {
        printf("[safe_charge] human sensor gpio init failed: %u\n", ret);
        g_human_sensor_ready = false;
        return;
    }

    ret = IoTGpioSetDir(SAFE_CHARGE_HUMAN_SENSOR_GPIO, IOT_GPIO_DIR_IN);
    if (ret != IOT_SUCCESS) {
        printf("[safe_charge] human sensor gpio dir failed: %u\n", ret);
        g_human_sensor_ready = false;
        return;
    }

    g_human_sensor_ready = true;
    printf("[safe_charge] human sensor ready on GPIO0_PA3\n");
}

bool human_sensor_read_present(void)
{
    IotGpioValue value = IOT_GPIO_VALUE0;

    if (!g_human_sensor_ready) {
        return false;
    }

    if (IoTGpioGetInputVal(SAFE_CHARGE_HUMAN_SENSOR_GPIO, &value) != IOT_SUCCESS) {
        return false;
    }

#if SAFE_CHARGE_HUMAN_SENSOR_ACTIVE_HIGH
    return value == IOT_GPIO_VALUE1;
#else
    return value == IOT_GPIO_VALUE0;
#endif
}
