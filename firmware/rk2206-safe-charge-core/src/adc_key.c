

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "adc_key.h"
#include "iot_adc.h"
#include "iot_errno.h"
#include "los_task.h"
#include "ohos_init.h"
#include "smart_home.h"

#define KEY_ADC_CHANNEL 7

#define KEY_SCAN_INTERVAL_MS 20
#define KEY_DEBOUNCE_TICKS 1

#define KEY_RELEASE_MIN_VOLTAGE 3.20f
#define KEY_LEFT_MIN_VOLTAGE 1.50f
#define KEY_DOWN_MIN_VOLTAGE 1.00f
#define KEY_RIGHT_MIN_VOLTAGE 0.50f
#define KEY_DEBUG_TICKS (1000 / KEY_SCAN_INTERVAL_MS)

static unsigned int adc_dev_init(void)
{
    unsigned int ret = IoTAdcInit(KEY_ADC_CHANNEL);

    if (ret != IOT_SUCCESS) {
        printf("%s, %s, %d: ADC Init fail\n", __FILE__, __func__, __LINE__);
    }

    return ret;
}

static float adc_get_voltage(void)
{
    unsigned int ret = IOT_SUCCESS;
    unsigned int data = 0;

    ret = IoTAdcGetVal(KEY_ADC_CHANNEL, &data);
    if (ret != IOT_SUCCESS) {
        printf("%s, %s, %d: ADC Read Fail\n", __FILE__, __func__, __LINE__);
        return 0.0f;
    }

    return (float)(data * 3.3f / 1024.0f);
}

static uint8_t adc_key_classify(float voltage)
{
    if (voltage >= KEY_RELEASE_MIN_VOLTAGE) {
        return KEY_RELEASE;
    }
    if (voltage > KEY_LEFT_MIN_VOLTAGE) {
        return KEY_LEFT;
    }
    if (voltage > KEY_DOWN_MIN_VOLTAGE) {
        return KEY_DOWN;
    }
    if (voltage > KEY_RIGHT_MIN_VOLTAGE) {
        return KEY_RIGHT;
    }
    return KEY_UP;
}

void adc_key_thread(uint32_t arg)
{
    (void)arg;

    float voltage;
    uint8_t raw_key = KEY_RELEASE;
    uint8_t last_raw_key = KEY_RELEASE;
    uint8_t stable_key = KEY_RELEASE;
    uint8_t active_key = KEY_RELEASE;
    uint8_t debounce_ticks = 0;
    uint8_t debug_ticks = 0;

    adc_dev_init();

    while (1) {
        voltage = adc_get_voltage();
        raw_key = adc_key_classify(voltage);
        debug_ticks++;
        if (debug_ticks >= KEY_DEBUG_TICKS) {
            debug_ticks = 0;
            printf("[safe_charge] key adc v=%.3fV, raw=%u, stable=%u\n",
                voltage, raw_key, stable_key);
        }

        if (raw_key == last_raw_key) {
            if (debounce_ticks < KEY_DEBOUNCE_TICKS) {
                debounce_ticks++;
            }
        } else {
            last_raw_key = raw_key;
            debounce_ticks = 0;
        }

        if (debounce_ticks >= KEY_DEBOUNCE_TICKS && raw_key != stable_key) {
            stable_key = raw_key;

            if (stable_key == KEY_RELEASE) {
                if (active_key != KEY_RELEASE) {
                    printf("[safe_charge] key release=%u, v=%.3fV\n",
                        active_key, voltage);
                }
                active_key = KEY_RELEASE;
            } else if (active_key == KEY_RELEASE) {
                active_key = stable_key;
                printf("[safe_charge] key press=%u, v=%.3fV\n",
                    active_key, voltage);
                smart_home_key_fast_action(active_key);
            } else {
                printf("[safe_charge] key ignored while holding old=%u new=%u, v=%.3fV\n",
                    active_key, stable_key, voltage);
            }
        } else if (stable_key == KEY_RELEASE && active_key != KEY_RELEASE) {
            if (raw_key == KEY_RELEASE) {
                printf("[safe_charge] key release, v=%.3fV\n", voltage);
                active_key = KEY_RELEASE;
            }
        }

        LOS_Msleep(KEY_SCAN_INTERVAL_MS);
    }
}

void adc_example(void)
{
    unsigned int thread_id;
    TSK_INIT_PARAM_S task = {0};
    unsigned int ret = LOS_OK;

    task.pfnTaskEntry = (TSK_ENTRY_FUNC)adc_key_thread;
    task.uwStackSize = 2048;
    task.pcName = "adc process";
    task.usTaskPrio = 24;
    ret = LOS_TaskCreate(&thread_id, &task);
    if (ret != LOS_OK) {
        printf("Falied to create task ret:0x%x\n", ret);
        return;
    }
}
