#ifndef GAS_SENSOR_H
#define GAS_SENSOR_H

#include <stdbool.h>

void gas_sensor_init(void);
float gas_sensor_read_ppm(void);
bool gas_sensor_is_ready(void);

#endif
