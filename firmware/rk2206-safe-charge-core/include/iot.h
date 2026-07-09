#ifndef _IOT_H_
#define _IOT_H_

#include <stdbool.h>

#include "charge_safety.h"

typedef struct
{
    double illumination;
    double temperature;
    double humidity;
    bool motor_state;
    bool light_state;
    bool auto_state;
} e_iot_data;

#define IOT_CMD_LIGHT_ON 0x01
#define IOT_CMD_LIGHT_OFF 0x02
#define IOT_CMD_MOTOR_ON 0x03
#define IOT_CMD_MOTOR_OFF 0x04
#define IOT_CMD_AUTO_ON 0x05
#define IOT_CMD_AUTO_OFF 0x06
#define IOT_CMD_CHARGE_START 0x20
#define IOT_CMD_CHARGE_STOP 0x21
#define IOT_CMD_CHARGE_RESET 0x22
#define IOT_CMD_CHARGE_CUTOFF 0x23
#define IOT_CMD_CHARGE_SIM_SMOKE 0x24
#define IOT_CMD_CHARGE_CLEAR_SMOKE 0x25
#define IOT_CMD_CHARGE_SIM_FULL 0x26
#define IOT_CMD_CHARGE_SIM_TEMP_WARN 0x27
#define IOT_CMD_CHARGE_SIM_CURRENT_WARN 0x28
#define IOT_CMD_CHARGE_SIM_GAS_WARN 0x29
#define IOT_CMD_CHARGE_DEMO_NEXT 0x2A
#define IOT_CMD_CHARGE_AUTH_OK 0x2B
#define IOT_CMD_CHARGE_AUTH_DENY 0x2C

int wait_message();
void iot_request_connect(void);
unsigned int iot_connection_requested(void);
void mqtt_init();
unsigned int mqtt_is_connected();
void send_msg_to_mqtt(e_iot_data *iot_data);
void send_safe_charge_msg_to_mqtt(const charge_safety_data_t *charge_data);

#endif
