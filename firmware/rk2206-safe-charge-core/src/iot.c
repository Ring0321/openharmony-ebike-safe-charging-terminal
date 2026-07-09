

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "charge_safety.h"
#include "charge_feedback.h"
#include "MQTTClient.h"
#include "cJSON.h"
#include "cmsis_os2.h"
#include "config_network.h"
#include "iot.h"
#include "los_task.h"
#include "ohos_init.h"
#include "relay_control.h"
#include "smart_home_event.h"

#define HOST_ADDR "YOUR_IOTDA_HOST"

#define CLIENT_ID "YOUR_IOTDA_CLIENT_ID"
#define DEVICE_ID  "YOUR_IOTDA_DEVICE_ID"
#define MQTT_DEVICES_PWD "YOUR_IOTDA_DEVICE_SECRET"

#define PUBLISH_TOPIC "$oc/devices/" DEVICE_ID "/sys/properties/report"
#define SUBCRIB_TOPIC                                                          \
  "$oc/devices/" DEVICE_ID "/sys/commands/#"
#define RESPONSE_TOPIC                                                         \
  "$oc/devices/" DEVICE_ID "/sys/commands/response"

#define MAX_BUFFER_LENGTH 2048
#define MAX_STRING_LENGTH 64

static unsigned char sendBuf[MAX_BUFFER_LENGTH];
static unsigned char readBuf[MAX_BUFFER_LENGTH];
static char smartHomePayload[MAX_BUFFER_LENGTH];
static char safeChargePayload[MAX_BUFFER_LENGTH];

Network network;
MQTTClient client;

static char mqtt_devid[64]=DEVICE_ID;
static char mqtt_pwd[80]=MQTT_DEVICES_PWD;
static char mqtt_username[64]=DEVICE_ID;
static char mqtt_hostaddr[64]=HOST_ADDR;

static char publish_topic[128] = PUBLISH_TOPIC;
static char subcribe_topic[128] = SUBCRIB_TOPIC;
static char response_topic[128] = RESPONSE_TOPIC;

static unsigned int mqttConnectFlag = 0;
static volatile unsigned int iotConnectRequested = 0;

extern bool motor_state;
extern bool light_state;
extern bool auto_state;

void iot_request_connect(void)
{
  if (iotConnectRequested == 0) {
    printf("[safe_charge] IoTDA connect requested\n");
  }
  iotConnectRequested = 1;
}

unsigned int iot_connection_requested(void)
{
  return iotConnectRequested;
}

static void send_iot_command(int cmd)
{
  event_info_t event = {0};
  event.event = event_iot_cmd;
  event.data.iot_data = cmd;
  smart_home_event_send(&event);
}

static const char *safe_charge_relay_status(safe_charge_relay_channel_t channel)
{
  return relay_get_channel_state(channel) ? "ON" : "OFF";
}

void send_msg_to_mqtt(e_iot_data *iot_data) {
  int rc;
  MQTTMessage message;
  char str[MAX_STRING_LENGTH] = {0};
  char *payload = smartHomePayload;

  if (mqttConnectFlag == 0) {
    printf("mqtt not connect\n");
    return;
  }
  memset(payload, 0, MAX_BUFFER_LENGTH);

  cJSON *root = cJSON_CreateObject();
  if (root != NULL) {
    cJSON *serv_arr = cJSON_AddArrayToObject(root, "services");
    cJSON *arr_item = cJSON_CreateObject();
    cJSON_AddStringToObject(arr_item, "service_id", "smartHome");
    cJSON *pro_obj = cJSON_CreateObject();
    cJSON_AddItemToObject(arr_item, "properties", pro_obj);

    memset(str, 0, sizeof(str));

    sprintf(str, "%5.2fLux", iot_data->illumination);
    cJSON_AddStringToObject(pro_obj, "illumination", str);

    sprintf(str, "%5.2f℃", iot_data->temperature);
    cJSON_AddStringToObject(pro_obj, "temperature", str);

    sprintf(str, "%5.2f%%", iot_data->humidity);
    cJSON_AddStringToObject(pro_obj, "humidity", str);

    if (iot_data->motor_state == true) {
      cJSON_AddStringToObject(pro_obj, "motorStatus", "ON");
    } else {
      cJSON_AddStringToObject(pro_obj, "motorStatus", "OFF");
    }

    if (iot_data->light_state == true) {
      cJSON_AddStringToObject(pro_obj, "lightStatus", "ON");
    } else {
      cJSON_AddStringToObject(pro_obj, "lightStatus", "OFF");
    }

    if (iot_data->auto_state == true) {
      cJSON_AddStringToObject(pro_obj, "autoStatus", "ON");
    } else {
      cJSON_AddStringToObject(pro_obj, "autoStatus", "OFF");
    }

    cJSON_AddItemToArray(serv_arr, arr_item);

    char *palyload_str = cJSON_PrintUnformatted(root);
    if (palyload_str != NULL) {
      strncpy(payload, palyload_str, MAX_BUFFER_LENGTH - 1);
      payload[MAX_BUFFER_LENGTH - 1] = '\0';
      cJSON_free(palyload_str);
    }
    cJSON_Delete(root);
  }

  message.qos = 0;
  message.retained = 0;
  message.payload = payload;
  message.payloadlen = strlen(payload);

  sprintf(publish_topic,"$oc/devices/%s/sys/properties/report",mqtt_devid);
  if ((rc = MQTTPublish(&client, publish_topic, &message)) != 0) {
    printf("Return code from MQTT publish is %d\n", rc);
    mqttConnectFlag = 0;
  } else {
    printf("mqtt publish success:%s\n", payload);
  }
}

void send_safe_charge_msg_to_mqtt(const charge_safety_data_t *charge_data) {
  int rc;
  MQTTMessage message;
  char *payload = safeChargePayload;

  if (charge_data == NULL) {
    return;
  }

  if (mqttConnectFlag == 0) {
    printf("mqtt not connect\n");
    return;
  }
  memset(payload, 0, MAX_BUFFER_LENGTH);

  cJSON *root = cJSON_CreateObject();
  if (root != NULL) {
    cJSON *serv_arr = cJSON_AddArrayToObject(root, "services");
    cJSON *arr_item = cJSON_CreateObject();
    cJSON_AddStringToObject(arr_item, "service_id", "safeCharge");
    cJSON *pro_obj = cJSON_CreateObject();
    cJSON_AddItemToObject(arr_item, "properties", pro_obj);

    cJSON_AddStringToObject(pro_obj, "deviceId", charge_data->device_id);
    cJSON_AddStringToObject(pro_obj, "portId", charge_data->port_id);
    cJSON_AddStringToObject(pro_obj, "userId", charge_data->user_id);
    cJSON_AddStringToObject(pro_obj, "sessionId", charge_data->session_id);
    cJSON_AddStringToObject(pro_obj, "chargeState",
                            charge_state_to_string(charge_data->charge_state));
    cJSON_AddStringToObject(pro_obj, "riskLevel",
                            risk_level_to_string(charge_data->risk_level));
    cJSON_AddNumberToObject(pro_obj, "riskScore", charge_data->risk_score);
    cJSON_AddStringToObject(pro_obj, "lastEvent",
                            charge_event_to_string(charge_data->last_event));
    cJSON_AddStringToObject(pro_obj, "eventTrace", charge_data->event_trace);
    cJSON_AddNumberToObject(pro_obj, "voltageV", charge_data->power.voltage_v);
    cJSON_AddNumberToObject(pro_obj, "currentA", charge_data->power.current_a);
    cJSON_AddNumberToObject(pro_obj, "powerW", charge_data->power.power_w);
    cJSON_AddNumberToObject(pro_obj, "energyWh", charge_data->energy_wh);
    cJSON_AddNumberToObject(pro_obj, "costEstimate", charge_data->cost_estimate);
    cJSON_AddNumberToObject(pro_obj, "temperatureC", charge_data->temperature_c);
    cJSON_AddNumberToObject(pro_obj, "humidityPct", charge_data->humidity_pct);
    cJSON_AddNumberToObject(pro_obj, "smokeLevel", charge_data->smoke_level);
    cJSON_AddStringToObject(pro_obj, "humanPresence",
                            charge_data->human_present ? "PRESENT" : "NONE");
    cJSON_AddStringToObject(pro_obj, "accessAuthorized",
                            charge_data->access_authorized ? "YES" : "NO");
    cJSON_AddStringToObject(pro_obj, "relayStatus",
                            charge_data->relay_state ? "ON" : "OFF");
    cJSON_AddStringToObject(pro_obj, "relayK1Status",
                            safe_charge_relay_status(SAFE_CHARGE_RELAY_K1_OUTPUT));
    cJSON_AddStringToObject(pro_obj, "relayK2Status",
                            safe_charge_relay_status(SAFE_CHARGE_RELAY_K2_COOLING));
    cJSON_AddStringToObject(pro_obj, "relayK3Status",
                            safe_charge_relay_status(SAFE_CHARGE_RELAY_K3_WARNING));
    cJSON_AddStringToObject(pro_obj, "relayK4Status",
                            safe_charge_relay_status(SAFE_CHARGE_RELAY_K4_PROTECTION));
    cJSON_AddStringToObject(pro_obj, "relayK1Role",
                            relay_get_channel_role(SAFE_CHARGE_RELAY_K1_OUTPUT));
    cJSON_AddStringToObject(pro_obj, "relayK2Role",
                            relay_get_channel_role(SAFE_CHARGE_RELAY_K2_COOLING));
    cJSON_AddStringToObject(pro_obj, "relayK3Role",
                            relay_get_channel_role(SAFE_CHARGE_RELAY_K3_WARNING));
    cJSON_AddStringToObject(pro_obj, "relayK4Role",
                            relay_get_channel_role(SAFE_CHARGE_RELAY_K4_PROTECTION));
    cJSON_AddStringToObject(pro_obj, "chargeMotorStatus",
                            charge_feedback_motor_status());
    cJSON_AddStringToObject(pro_obj, "alarmStatus",
                            charge_feedback_alarm_active() ? "ON" : "OFF");
    cJSON_AddStringToObject(pro_obj, "coolingStatus",
                            charge_feedback_cooling_status());
    cJSON_AddStringToObject(pro_obj, "lightStatus",
                            charge_feedback_light_status());
    cJSON_AddNumberToObject(pro_obj, "demoStep",
                            charge_feedback_demo_step(charge_data->charge_state));
    cJSON_AddStringToObject(pro_obj, "networkStatus",
                            charge_data->network_state ? "ONLINE" : "OFFLINE");
    cJSON_AddStringToObject(pro_obj, "powerSensor",
                            charge_data->power.online ? "INA219" :
                            (charge_data->sensor_simulated ? "SIMULATED" : "OFFLINE"));
    cJSON_AddNumberToObject(pro_obj, "eventCount", charge_data->event_count);
    cJSON_AddNumberToObject(pro_obj, "faultCode", charge_data->fault_code);

    charge_thresholds_t thresholds;
    charge_safety_get_thresholds(&thresholds);
    cJSON_AddNumberToObject(pro_obj, "temperatureLimit", thresholds.temperature_limit);
    cJSON_AddNumberToObject(pro_obj, "currentLimit", thresholds.current_limit);
    cJSON_AddNumberToObject(pro_obj, "smokeLimit", thresholds.smoke_limit);

    cJSON_AddItemToArray(serv_arr, arr_item);

    char *payload_str = cJSON_PrintUnformatted(root);
    if (payload_str != NULL) {
      strncpy(payload, payload_str, MAX_BUFFER_LENGTH - 1);
      payload[MAX_BUFFER_LENGTH - 1] = '\0';
      cJSON_free(payload_str);
    }
    cJSON_Delete(root);
  }

  message.qos = 0;
  message.retained = 0;
  message.payload = payload;
  message.payloadlen = strlen(payload);

  sprintf(publish_topic, "$oc/devices/%s/sys/properties/report", mqtt_devid);
  if ((rc = MQTTPublish(&client, publish_topic, &message)) != 0) {
    printf("Return code from safe charge MQTT publish is %d\n", rc);
    mqttConnectFlag = 0;
  } else {
    printf("safe charge mqtt publish success:%s\n", payload);
  }
}

void set_light_state(cJSON *root) {
  cJSON *para_obj = NULL;
  cJSON *status_obj = NULL;
  char *value = NULL;

  event_info_t event={0};
  event.event=event_iot_cmd;

  para_obj = cJSON_GetObjectItem(root, "paras");
  status_obj = cJSON_GetObjectItem(para_obj, "onoff");
  if (status_obj != NULL) {
    value = cJSON_GetStringValue(status_obj);
    if (!strcmp(value, "ON")) {
      event.data.iot_data = IOT_CMD_LIGHT_ON;

    } else if (!strcmp(value, "OFF")) {
      event.data.iot_data = IOT_CMD_LIGHT_OFF;

    }
    smart_home_event_send(&event);
  }
}

void set_motor_state(cJSON *root) {
  cJSON *para_obj = NULL;
  cJSON *status_obj = NULL;
  char *value = NULL;

  event_info_t event={0};
  event.event=event_iot_cmd;

  para_obj = cJSON_GetObjectItem(root, "paras");
  status_obj = cJSON_GetObjectItem(para_obj, "onoff");
  if (status_obj != NULL) {
    value = cJSON_GetStringValue(status_obj);
    if (!strcmp(value, "ON")) {

      event.data.iot_data = IOT_CMD_MOTOR_ON;
    } else if (!strcmp(value, "OFF")) {

      event.data.iot_data = IOT_CMD_MOTOR_OFF;
    }
    smart_home_event_send(&event);
  }
}

void set_auto_state(cJSON *root) {
  cJSON *para_obj = NULL;
  cJSON *status_obj = NULL;
  char *value = NULL;

  para_obj = cJSON_GetObjectItem(root, "paras");
  status_obj = cJSON_GetObjectItem(para_obj, "onoff");
  if (status_obj != NULL) {
    value = cJSON_GetStringValue(status_obj);
    if (!strcmp(value, "ON")) {

    } else if (!strcmp(value, "OFF")) {

    }
  }
}

static int safe_charge_action_to_cmd(const char *action)
{
  if (action == NULL) {
    return 0;
  }
  if (!strcmp(action, "start_demo") || !strcmp(action, "start_charge") ||
      !strcmp(action, "authorize") || !strcmp(action, "auth_ok") ||
      !strcmp(action, "access_grant")) {
    return IOT_CMD_CHARGE_AUTH_OK;
  }
  if (!strcmp(action, "auth_deny") || !strcmp(action, "deny") ||
      !strcmp(action, "access_deny")) {
    return IOT_CMD_CHARGE_AUTH_DENY;
  }
  if (!strcmp(action, "stop_charge")) {
    return IOT_CMD_CHARGE_STOP;
  }
  if (!strcmp(action, "reset_alarm")) {
    return IOT_CMD_CHARGE_RESET;
  }
  if (!strcmp(action, "relay_off") || !strcmp(action, "remote_cutoff")) {
    return IOT_CMD_CHARGE_CUTOFF;
  }
  if (!strcmp(action, "simulate_smoke")) {
    return IOT_CMD_CHARGE_SIM_SMOKE;
  }
  if (!strcmp(action, "clear_smoke")) {
    return IOT_CMD_CHARGE_CLEAR_SMOKE;
  }
  if (!strcmp(action, "simulate_full")) {
    return IOT_CMD_CHARGE_SIM_FULL;
  }
  if (!strcmp(action, "simulate_temp_warn") || !strcmp(action, "sim_temp_warn")) {
    return IOT_CMD_CHARGE_SIM_TEMP_WARN;
  }
  if (!strcmp(action, "simulate_current_warn") || !strcmp(action, "sim_current_warn")) {
    return IOT_CMD_CHARGE_SIM_CURRENT_WARN;
  }
  if (!strcmp(action, "simulate_gas_warn") || !strcmp(action, "sim_gas_warn")) {
    return IOT_CMD_CHARGE_SIM_GAS_WARN;
  }
  if (!strcmp(action, "demo_next")) {
    return IOT_CMD_CHARGE_DEMO_NEXT;
  }
  return 0;
}

static bool safe_charge_read_float(cJSON *obj, const char *name, float *value)
{
  cJSON *item = cJSON_GetObjectItem(obj, name);
  if (item == NULL || item->type != cJSON_Number) {
    return false;
  }
  *value = (float)item->valuedouble;
  return true;
}

static bool safe_charge_update_thresholds(cJSON *para_obj)
{
  bool updated = false;
  float value = 0.0f;
  charge_thresholds_t thresholds;

  if (para_obj == NULL) {
    return false;
  }

  charge_safety_get_thresholds(&thresholds);

  if (safe_charge_read_float(para_obj, "temperature_limit", &value) ||
      safe_charge_read_float(para_obj, "temperatureLimit", &value)) {
    thresholds.temperature_limit = value;
    updated = true;
  }
  if (safe_charge_read_float(para_obj, "temperature_cutoff", &value) ||
      safe_charge_read_float(para_obj, "temperatureCutoff", &value)) {
    thresholds.temperature_cutoff = value;
    updated = true;
  }
  if (safe_charge_read_float(para_obj, "current_limit", &value) ||
      safe_charge_read_float(para_obj, "currentLimit", &value)) {
    thresholds.current_limit = value;
    updated = true;
  }
  if (safe_charge_read_float(para_obj, "current_cutoff", &value) ||
      safe_charge_read_float(para_obj, "currentCutoff", &value)) {
    thresholds.current_cutoff = value;
    updated = true;
  }
  if (safe_charge_read_float(para_obj, "smoke_limit", &value) ||
      safe_charge_read_float(para_obj, "smokeLimit", &value)) {
    thresholds.smoke_limit = value;
    updated = true;
  }
  if (safe_charge_read_float(para_obj, "smoke_cutoff", &value) ||
      safe_charge_read_float(para_obj, "smokeCutoff", &value)) {
    thresholds.smoke_cutoff = value;
    updated = true;
  }
  if (safe_charge_read_float(para_obj, "full_current", &value) ||
      safe_charge_read_float(para_obj, "fullCurrent", &value)) {
    thresholds.full_current = value;
    updated = true;
  }

  if (updated) {
    charge_safety_set_thresholds(&thresholds);
    printf("[safe_charge] thresholds updated temp=%.2f current=%.2f smoke=%.2f\n",
      thresholds.temperature_limit, thresholds.current_limit, thresholds.smoke_limit);
  }
  return updated;
}

void set_safe_charge_state(cJSON *root) {
  cJSON *para_obj = NULL;
  cJSON *action_obj = NULL;
  char *action = NULL;
  int cmd = 0;

  para_obj = cJSON_GetObjectItem(root, "paras");
  if (para_obj == NULL) {
    return;
  }

  action_obj = cJSON_GetObjectItem(para_obj, "action");
  if (action_obj == NULL) {
    action_obj = cJSON_GetObjectItem(para_obj, "cmd_value");
  }
  if (action_obj == NULL) {
    return;
  }

  action = cJSON_GetStringValue(action_obj);
  if (action != NULL && !strcmp(action, "set_thresholds")) {
    safe_charge_update_thresholds(para_obj);
    return;
  }

  cmd = safe_charge_action_to_cmd(action);
  if (cmd != 0) {
    send_iot_command(cmd);
  }
}

void mqtt_message_arrived(MessageData *data) {
  int rc;
  cJSON *root = NULL;
  cJSON *cmd_name = NULL;
  char *cmd_name_str = NULL;
  char *request_id_idx = NULL;
  char request_id[40] = {0};
  MQTTMessage message;
  char payload[160] = {0};

  char rsptopic[128] = {0};

  printf("Message arrived on topic %.*s: %.*s\n",
         data->topicName->lenstring.len, data->topicName->lenstring.data,
         data->message->payloadlen, data->message->payload);


  request_id_idx = strstr(data->topicName->lenstring.data, "request_id=");
  if (request_id_idx != NULL) {
    strncpy(request_id, request_id_idx + 11, 36);
  }



  sprintf(response_topic,"$oc/devices/%s/sys/commands/response",mqtt_devid);
  sprintf(rsptopic, "%s/request_id=%s", response_topic, request_id);



  message.qos = 0;
  message.retained = 0;
  message.payload = payload;
  sprintf(payload, "{ \
    \"result_code\": 0, \
    \"response_name\": \"COMMAND_RESPONSE\", \
    \"paras\": { \
        \"result\": \"success\" \
    } \
    }");
  message.payloadlen = strlen(payload);


  if ((rc = MQTTPublish(&client, rsptopic, &message)) != 0) {
    printf("Return code from MQTT publish is %d\n", rc);
    mqttConnectFlag = 0;
  }


  root =
      cJSON_ParseWithLength(data->message->payload, data->message->payloadlen);
  if (root != NULL) {
    cmd_name = cJSON_GetObjectItem(root, "command_name");
    if (cmd_name != NULL) {
      cmd_name_str = cJSON_GetStringValue(cmd_name);
      if (!strcmp(cmd_name_str, "light_control")) {
        set_light_state(root);
      } else if (!strcmp(cmd_name_str, "motor_control")) {
        set_motor_state(root);
      } else if (!strcmp(cmd_name_str, "auto_control")) {
        set_auto_state(root);
      } else if (!strcmp(cmd_name_str, "safe_charge_control")) {
        set_safe_charge_state(root);
      } else if (!strcmp(cmd_name_str, "set_thresholds")) {
        cJSON *para_obj = cJSON_GetObjectItem(root, "paras");
        safe_charge_update_thresholds(para_obj);
      } else {
        int safe_cmd = safe_charge_action_to_cmd(cmd_name_str);
        if (safe_cmd != 0) {
          send_iot_command(safe_cmd);
        }
      }
    }
  }

  cJSON_Delete(root);
}

int wait_message() {
  uint8_t rec = MQTTYield(&client, 5000);
  if (rec != 0) {
    mqttConnectFlag = 0;
  }
  if (mqttConnectFlag == 0) {
    return 0;
  }
  return 1;
}

void mqtt_init() {
  int rc;

  printf("Starting MQTT...\n");


  NetworkInit(&network);

begin:

  printf("NetworkConnect  ...\n");
  NetworkConnect(&network, HOST_ADDR, 1883);
  printf("MQTTClientInit  ...\n");

  MQTTClientInit(&client, &network, 2000, sendBuf, sizeof(sendBuf), readBuf,
                 sizeof(readBuf));

  MQTTString clientId = MQTTString_initializer;
  clientId.cstring = CLIENT_ID;

  MQTTString userName = MQTTString_initializer;
  userName.cstring = mqtt_username;

  MQTTString password = MQTTString_initializer;
  password.cstring = mqtt_pwd;

  MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
  data.clientID = clientId;
  data.username = userName;
  data.password = password;
  data.willFlag = 0;
  data.MQTTVersion = 4;
  data.keepAliveInterval = 60;
  data.cleansession = 1;

  printf("MQTTConnect  ...\n");
  rc = MQTTConnect(&client, &data);
  if (rc != 0) {
    printf("MQTTConnect: %d\n", rc);
    NetworkDisconnect(&network);
    MQTTDisconnect(&client);
    osDelay(200);
    goto begin;
  }

  printf("MQTTSubscribe  ...\n");
  sprintf(subcribe_topic,"$oc/devices/%s/sys/commands/#",mqtt_devid);
  rc = MQTTSubscribe(&client, subcribe_topic, 0, mqtt_message_arrived);
  if (rc != 0) {
    printf("MQTTSubscribe: %d\n", rc);
    osDelay(200);
    goto begin;
  }

  mqttConnectFlag = 1;
}

unsigned int mqtt_is_connected() { return mqttConnectFlag; }
