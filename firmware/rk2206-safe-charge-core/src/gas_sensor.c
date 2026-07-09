#include "gas_sensor.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "iot_adc.h"
#include "iot_errno.h"
#include "los_task.h"

#define MQ2_ADC_CHANNEL 4
#define MQ2_CLEAN_AIR_PPM 20.0f
#define MQ2_LOAD_RESISTANCE_KOHM 1.0f
#define MQ2_SUPPLY_VOLTAGE 5.0f
#define MQ2_ADC_REF_VOLTAGE 3.3f
#define MQ2_ADC_RESOLUTION 1024.0f
#define MQ2_CALIBRATION_SAMPLES 8
#define MQ2_SAMPLE_DELAY_MS 50
#define MQ2_FILTER_ALPHA 0.35f

static float g_mq2_r0 = 0.0f;
static float g_mq2_filtered_ppm = 0.0f;
static bool g_mq2_ready = false;
static bool g_mq2_filter_ready = false;

static float gas_sensor_read_voltage(void)
{
    unsigned int raw = 0;
    unsigned int ret = IoTAdcGetVal(MQ2_ADC_CHANNEL, &raw);

    if (ret != IOT_SUCCESS) {
        printf("[safe_charge] MQ2 ADC read failed: %u\n", ret);
        return 0.0f;
    }

    return ((float)raw * MQ2_ADC_REF_VOLTAGE) / MQ2_ADC_RESOLUTION;
}

static float gas_sensor_calc_rs(float voltage)
{
    if (voltage <= 0.01f) {
        return 0.0f;
    }

    return (MQ2_SUPPLY_VOLTAGE - voltage) / voltage * MQ2_LOAD_RESISTANCE_KOHM;
}

static void gas_sensor_calibrate(void)
{
    float voltage_sum = 0.0f;
    float voltage = 0.0f;
    float rs = 0.0f;
    uint8_t sample_count = 0;

    for (uint8_t i = 0; i < MQ2_CALIBRATION_SAMPLES; i++) {
        float sample_voltage = gas_sensor_read_voltage();

        if (sample_voltage > 0.01f) {
            voltage_sum += sample_voltage;
            sample_count++;
        }
        LOS_Msleep(MQ2_SAMPLE_DELAY_MS);
    }

    if (sample_count == 0) {
        g_mq2_ready = false;
        printf("[safe_charge] MQ2 calibration failed, no valid sample\n");
        return;
    }

    voltage = voltage_sum / sample_count;
    rs = gas_sensor_calc_rs(voltage);

    if (rs <= 0.0f) {
        g_mq2_ready = false;
        printf("[safe_charge] MQ2 calibration failed, voltage=%.3f\n", voltage);
        return;
    }

    g_mq2_r0 = rs / powf(MQ2_CLEAN_AIR_PPM / 613.9f, 1.0f / -2.074f);
    g_mq2_ready = (g_mq2_r0 > 0.0f);
    g_mq2_filter_ready = false;
    printf("[safe_charge] MQ2 calibrated voltage=%.3f r0=%.3f samples=%u\n",
        voltage, g_mq2_r0, sample_count);
}

void gas_sensor_init(void)
{
    unsigned int ret = IoTAdcInit(MQ2_ADC_CHANNEL);

    g_mq2_ready = false;
    g_mq2_r0 = 0.0f;
    g_mq2_filtered_ppm = 0.0f;
    g_mq2_filter_ready = false;

    if (ret != IOT_SUCCESS) {
        printf("[safe_charge] MQ2 ADC init failed: %u\n", ret);
        return;
    }

    LOS_Msleep(1000);
    gas_sensor_calibrate();
}

float gas_sensor_read_ppm(void)
{
    float voltage = 0.0f;
    float rs = 0.0f;
    float ppm = 0.0f;

    if (!g_mq2_ready || g_mq2_r0 <= 0.0f) {
        return 0.0f;
    }

    voltage = gas_sensor_read_voltage();
    rs = gas_sensor_calc_rs(voltage);
    if (rs <= 0.0f) {
        return 0.0f;
    }

    ppm = 613.9f * powf(rs / g_mq2_r0, -2.074f);
    if (!g_mq2_filter_ready) {
        g_mq2_filtered_ppm = ppm;
        g_mq2_filter_ready = true;
    } else {
        g_mq2_filtered_ppm = g_mq2_filtered_ppm * (1.0f - MQ2_FILTER_ALPHA) +
            ppm * MQ2_FILTER_ALPHA;
    }

    return g_mq2_filtered_ppm;
}

bool gas_sensor_is_ready(void)
{
    return g_mq2_ready;
}
