#ifndef POWER_SENSOR_H
#define POWER_SENSOR_H

#include <stdbool.h>

typedef struct {
    float voltage_v;
    float current_a;
    float power_w;
    bool online;
    bool simulated;
} power_sensor_data_t;

void power_sensor_init(void);
void power_sensor_read(power_sensor_data_t *data, bool load_enabled);
bool power_sensor_is_online(void);

#endif
