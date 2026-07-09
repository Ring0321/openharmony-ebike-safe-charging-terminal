#include "power_sensor.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "iot_errno.h"
#include "iot_i2c.h"

#define SAFE_CHARGE_I2C_HANDLE EI2C0_M2
#define INA219_ADDRESS 0x40

#define INA219_REG_CONFIG 0x00
#define INA219_REG_BUS_VOLTAGE 0x02
#define INA219_REG_POWER 0x03
#define INA219_REG_CURRENT 0x04
#define INA219_REG_CALIBRATION 0x05

#define INA219_CALIBRATION_VALUE 4096
#define INA219_CURRENT_LSB_A 0.0001f
#define INA219_POWER_LSB_W 0.002f

static bool g_sensor_online = false;
static unsigned int g_sim_tick = 0;

static unsigned int ina219_write_u16(uint8_t reg, uint16_t value)
{
    uint8_t data[3] = {
        reg,
        (uint8_t)((value >> 8) & 0xFF),
        (uint8_t)(value & 0xFF),
    };
    return IoTI2cWrite(SAFE_CHARGE_I2C_HANDLE, INA219_ADDRESS, data, sizeof(data));
}

static unsigned int ina219_read_u16(uint8_t reg, uint16_t *value)
{
    uint8_t data[2] = {0};
    unsigned int ret = IoTI2cWrite(SAFE_CHARGE_I2C_HANDLE, INA219_ADDRESS, &reg, 1);
    if (ret != IOT_SUCCESS) {
        return ret;
    }

    ret = IoTI2cRead(SAFE_CHARGE_I2C_HANDLE, INA219_ADDRESS, data, sizeof(data));
    if (ret != IOT_SUCCESS) {
        return ret;
    }

    *value = ((uint16_t)data[0] << 8) | data[1];
    return IOT_SUCCESS;
}

void power_sensor_init(void)
{
    uint16_t config = 0;
    g_sensor_online = false;

    if (ina219_write_u16(INA219_REG_CONFIG, 0x399F) != IOT_SUCCESS) {
        printf("[safe_charge] INA219 not found, using simulated power data\n");
        return;
    }

    if (ina219_write_u16(INA219_REG_CALIBRATION, INA219_CALIBRATION_VALUE) != IOT_SUCCESS) {
        printf("[safe_charge] INA219 calibration failed, using simulated power data\n");
        return;
    }

    if (ina219_read_u16(INA219_REG_CONFIG, &config) != IOT_SUCCESS) {
        printf("[safe_charge] INA219 readback failed, using simulated power data\n");
        return;
    }

    g_sensor_online = true;
    printf("[safe_charge] INA219 online, config=0x%04x\n", config);
}

static void power_sensor_read_simulated(power_sensor_data_t *data, bool load_enabled)
{
    memset(data, 0, sizeof(*data));
    data->online = false;
    data->simulated = true;

    if (!load_enabled) {
        data->voltage_v = 5.00f;
        data->current_a = 0.00f;
        data->power_w = 0.00f;
        return;
    }

    g_sim_tick++;
    data->voltage_v = 5.00f;
    if (g_sim_tick < 10) {
        data->current_a = 0.52f;
    } else if (g_sim_tick < 20) {
        data->current_a = 0.35f;
    } else {
        data->current_a = 0.05f;
    }
    data->power_w = data->voltage_v * data->current_a;
}

void power_sensor_read(power_sensor_data_t *data, bool load_enabled)
{
    uint16_t raw_bus = 0;
    uint16_t raw_power = 0;
    uint16_t raw_current_u16 = 0;
    int16_t raw_current = 0;

    if (data == NULL) {
        return;
    }

    if (!g_sensor_online) {
        power_sensor_read_simulated(data, load_enabled);
        return;
    }

    if (ina219_write_u16(INA219_REG_CALIBRATION, INA219_CALIBRATION_VALUE) != IOT_SUCCESS ||
        ina219_read_u16(INA219_REG_BUS_VOLTAGE, &raw_bus) != IOT_SUCCESS ||
        ina219_read_u16(INA219_REG_CURRENT, &raw_current_u16) != IOT_SUCCESS ||
        ina219_read_u16(INA219_REG_POWER, &raw_power) != IOT_SUCCESS) {
        g_sensor_online = false;
        power_sensor_read_simulated(data, load_enabled);
        return;
    }

    raw_current = (int16_t)raw_current_u16;
    data->online = true;
    data->simulated = false;
    data->voltage_v = ((raw_bus >> 3) * 4.0f) / 1000.0f;
    data->current_a = raw_current * INA219_CURRENT_LSB_A;
    data->power_w = raw_power * INA219_POWER_LSB_W;
}

bool power_sensor_is_online(void)
{
    return g_sensor_online;
}
