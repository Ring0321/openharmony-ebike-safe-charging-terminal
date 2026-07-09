

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "los_task.h"
#include "ohos_init.h"
#include "cmsis_os.h"
#include "config_network.h"
#include "charge_feedback.h"
#include "charge_safety.h"
#include "gas_sensor.h"
#include "human_sensor.h"
#include "smart_home.h"
#include "smart_home_event.h"
#include "su_03t.h"
#include "iot.h"
#include "lcd.h"
#include "picture.h"
#include "adc_key.h"

#define ROUTE_SSID      "YOUR_WIFI_SSID"
#define ROUTE_PASSWORD "YOUR_WIFI_PASSWORD"

#define MSG_QUEUE_LENGTH                                16
#define BUFFER_LEN                                      50
#define SMART_HOME_LOOP_TIMEOUT_MS                      50
#define SMART_HOME_SENSOR_REFRESH_TICKS                 10

void iot_thread(void *args) {
  uint8_t mac_address[12] = {0x00, 0xdc, 0xb6, 0x90, 0x01, 0x00,0};

  char ssid[32]=ROUTE_SSID;
  char password[32]=ROUTE_PASSWORD;
  char mac_addr[32]={0};

  FlashDeinit();
  FlashInit();

  VendorSet(VENDOR_ID_WIFI_MODE, "STA", 3);
  VendorSet(VENDOR_ID_MAC, mac_address, 6);
  VendorSet(VENDOR_ID_WIFI_ROUTE_SSID, ssid, sizeof(ssid));
  VendorSet(VENDOR_ID_WIFI_ROUTE_PASSWD, password,sizeof(password));

  printf("[safe_charge] IoTDA thread ready, press K3 to connect WiFi\n");
  while (!iot_connection_requested()) {
    LOS_Msleep(100);
  }

reconnect:
  while (!iot_connection_requested()) {
    LOS_Msleep(100);
  }
  SetWifiModeOff();
  int ret = SetWifiModeOn();
  if(ret != 0){
    printf("wifi connect failed,please check wifi config and the AP!\n");
    LOS_Msleep(1000);
    goto reconnect;
  }
  mqtt_init();

  while (1) {
    if (!wait_message()) {
      LOS_Msleep(1000);
      goto reconnect;
    }
    LOS_Msleep(1);
  }
}

void smart_home_thread(void *arg)
{
    charge_safety_data_t charge_data = {0};
    uint8_t refresh_ticks = 0;

    printf("[safe_charge] smart_home_thread boot v20260612\n");

    i2c_dev_init();
    lcd_dev_init();
    lcd_fill(0, 0, LCD_W - 1, LCD_H - 1, LCD_WHITE);
    lcd_show_string(20, 20, (const uint8_t *)"SAFE CHARGE BOOT", LCD_RED, LCD_WHITE, 24, 0);
    lcd_show_string(20, 52, (const uint8_t *)"init devices...", LCD_BLUE, LCD_WHITE, 16, 0);
    printf("[safe_charge] i2c/lcd init done\n");

    gas_sensor_init();
    printf("[safe_charge] gas sensor init done\n");
    human_sensor_init();
    printf("[safe_charge] human sensor init done\n");

    motor_dev_init();
    light_dev_init();
    su03t_init();
    charge_feedback_init();
    printf("[safe_charge] motor/light/su03t init done\n");

    charge_safety_init();
    printf("[safe_charge] charge_safety_init done\n");
    lcd_show_ui();

    while(1)
    {
        event_info_t event_info = {0};
        bool handled_event = false;

        int ret = smart_home_event_wait(&event_info, SMART_HOME_LOOP_TIMEOUT_MS);
        while(ret == LOS_OK){
            handled_event = true;

            printf("event recv %d ,%d\n",event_info.event,event_info.data.iot_data);
            switch (event_info.event)
            {
                case event_key_press:
                    smart_home_key_process(event_info.data.key_no);

                    break;
                case event_iot_cmd:
                    smart_home_iot_cmd_process(event_info.data.iot_data);
                    break;
                case event_su03t:
                    smart_home_su03t_cmd_process(event_info.data.su03t_data);
                    break;
               default:break;
            }

            memset(&event_info, 0, sizeof(event_info));
            ret = smart_home_event_wait(&event_info, 0);
        }

        if (handled_event) {
            charge_safety_get_data(&charge_data);
            charge_feedback_apply(&charge_data);
            lcd_show_ui();
            refresh_ticks = 0;
            continue;
        }

        refresh_ticks++;
        if (refresh_ticks < SMART_HOME_SENSOR_REFRESH_TICKS) {
            continue;
        }
        refresh_ticks = 0;

        double temp,humi,lum;
        double smoke_ppm = 0.0;
        bool human_present = false;

        sht30_read_data(&temp,&humi);
        bh1750_read_data(&lum);
        smoke_ppm = gas_sensor_read_ppm();
        human_present = human_sensor_read_present();

        charge_safety_set_network_state(mqtt_is_connected() != 0);
        charge_safety_update(temp, humi, lum, smoke_ppm, human_present);
        charge_safety_get_data(&charge_data);
        charge_feedback_apply(&charge_data);
        if (mqtt_is_connected())
        {

            send_safe_charge_msg_to_mqtt(&charge_data);

            lcd_set_network_state(true);
        }else{
            lcd_set_network_state(false);
        }

        lcd_show_ui();
    }
}

void iot_smart_home_example()
{
    unsigned int thread_id_1;
    unsigned int thread_id_2;
    unsigned int thread_id_3;
    TSK_INIT_PARAM_S task_1 = {0};
    TSK_INIT_PARAM_S task_2 = {0};
    TSK_INIT_PARAM_S task_3 = {0};
    unsigned int ret = LOS_OK;

    printf("[safe_charge] app init v20260612\n");

    smart_home_event_init();








    task_1.pfnTaskEntry = (TSK_ENTRY_FUNC)smart_home_thread;
    task_1.uwStackSize = 8192;
    task_1.pcName = "smart home thread";
    task_1.usTaskPrio = 24;

    ret = LOS_TaskCreate(&thread_id_1, &task_1);
    if (ret != LOS_OK)
    {
        printf("Falied to create task ret:0x%x\n", ret);
        return;
    }

    task_2.pfnTaskEntry = (TSK_ENTRY_FUNC)adc_key_thread;
    task_2.uwStackSize = 2048;
    task_2.pcName = "key thread";
    task_2.usTaskPrio = 23;
    ret = LOS_TaskCreate(&thread_id_2, &task_2);
    if (ret != LOS_OK)
    {
        printf("Falied to create task ret:0x%x\n", ret);
        return;
    }

    task_3.pfnTaskEntry = (TSK_ENTRY_FUNC)iot_thread;
    task_3.uwStackSize = 20480*5;
    task_3.pcName = "iot thread";
    task_3.usTaskPrio = 24;
    ret = LOS_TaskCreate(&thread_id_3, &task_3);
    if (ret != LOS_OK)
    {
        printf("Falied to create task ret:0x%x\n", ret);
        return;
    }
}

APP_FEATURE_INIT(iot_smart_home_example);
