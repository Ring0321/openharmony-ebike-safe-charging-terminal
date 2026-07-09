

#include "smart_home.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "adc_key.h"
#include "charge_feedback.h"
#include "charge_safety.h"
#include "drv_light.h"
#include "drv_motor.h"
#include "iot.h"
#include "lcd.h"
#include "picture.h"
#include "safe_charge_assets.h"
#include "su_03t.h"

static bool g_network_state = false;
static bool g_ui_initialized = false;
static uint8_t g_banner_blink = 0;
static char g_last_key_text[32] = "KEY READY";
static uint8_t g_key_flash_ticks = 0;
static uint16_t g_key_flash_color = LCD_BLUE;

static uint16_t risk_color(risk_level_t risk)
{
    switch (risk) {
        case RISK_LEVEL_OK:
            return LCD_GREEN;
        case RISK_LEVEL_L1_REMIND:
            return LCD_BROWN;
        case RISK_LEVEL_L2_WARN:
            return LCD_YELLOW;
        case RISK_LEVEL_L3_PROTECT:
        case RISK_LEVEL_L4_FAULT:
            return LCD_RED;
        default:
            return LCD_BLACK;
    }
}

static const unsigned char *safe_charge_status_card(charge_state_t state)
{
    switch (state) {
        case CHARGE_STATE_IDLE:
            return img_sc_card_idle;
        case CHARGE_STATE_CHARGING:
            return img_sc_card_charging;
        case CHARGE_STATE_FULL:
            return img_sc_card_full;
        case CHARGE_STATE_OCCUPIED:
            return img_sc_card_occupied;
        case CHARGE_STATE_ALERT:
            return img_sc_card_alert;
        case CHARGE_STATE_CUT_OFF:
            return img_sc_card_cutoff;
        case CHARGE_STATE_FAULT:
            return img_sc_card_locked;
        default:
            return img_sc_card_idle;
    }
}

static uint16_t demo_theme_color(charge_state_t state)
{
    switch (state) {
        case CHARGE_STATE_CHARGING:
            return LCD_BROWN;
        case CHARGE_STATE_FULL:
            return LCD_BLUE;
        case CHARGE_STATE_OCCUPIED:
            return LCD_MAGENTA;
        case CHARGE_STATE_ALERT:
            return LCD_YELLOW;
        case CHARGE_STATE_CUT_OFF:
            return LCD_RED;
        case CHARGE_STATE_FAULT:
            return LCD_BLACK;
        case CHARGE_STATE_IDLE:
        default:
            return LCD_GREEN;
    }
}

static uint8_t demo_flow_step(charge_state_t state)
{
    switch (state) {
        case CHARGE_STATE_CHARGING:
            return 2;
        case CHARGE_STATE_FULL:
        case CHARGE_STATE_OCCUPIED:
        case CHARGE_STATE_ALERT:
            return 3;
        case CHARGE_STATE_CUT_OFF:
        case CHARGE_STATE_FAULT:
            return 4;
        case CHARGE_STATE_IDLE:
        default:
            return 1;
    }
}

static const char *demo_stage_text(charge_state_t state)
{
    switch (state) {
        case CHARGE_STATE_CHARGING:
            return "第2步 正在充电检测";
        case CHARGE_STATE_FULL:
            return "第3步 充满提醒";
        case CHARGE_STATE_OCCUPIED:
            return "第3步 占位提醒";
        case CHARGE_STATE_ALERT:
            return "第3步 风险告警";
        case CHARGE_STATE_CUT_OFF:
            return "第4步 断电保护";
        case CHARGE_STATE_FAULT:
            return "第4步 故障锁定";
        case CHARGE_STATE_IDLE:
        default:
            return "第1步 待机安全";
    }
}

static void lcd_show_mixed_text(uint16_t x, uint16_t y, const char *text,
    uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
    const uint8_t *p = (const uint8_t *)text;
    uint8_t cn[4] = {0};

    while (*p != '\0') {
        if (*p < 0x80) {
            lcd_show_char(x, y, *p, fc, bc, sizey, mode);
            x += sizey / 2;
            p++;
        } else if (p[1] != '\0' && p[2] != '\0') {
            cn[0] = p[0];
            cn[1] = p[1];
            cn[2] = p[2];
            cn[3] = '\0';
            lcd_show_chinese(x, y, cn, fc, bc, sizey, mode);
            x += sizey;
            p += 3;
        } else {
            break;
        }

        if (x > LCD_W - sizey) {
            break;
        }
    }
}

static void lcd_show_demo_title(void)
{
    lcd_fill(0, 0, LCD_W - 1, 50, LCD_WHITE);
    lcd_show_mixed_text(8, 8, "社区安全充电", LCD_DARKBLUE, LCD_WHITE, 24, 0);
    lcd_draw_line(8, 36, 154, 36, LCD_DARKBLUE);
    lcd_draw_line(8, 38, 118, 38, LCD_LIGHTBLUE);
}

static void lcd_show_stage_panel(const charge_safety_data_t *data)
{
    uint16_t theme = demo_theme_color(data->charge_state);
    uint16_t bg = LCD_WHITE;
    uint16_t fg = theme;
    char risk_text[20] = {0};
    bool strong = (data->charge_state == CHARGE_STATE_ALERT ||
        data->charge_state == CHARGE_STATE_CUT_OFF ||
        data->charge_state == CHARGE_STATE_FAULT);

    if (strong) {
        g_banner_blink++;
        bg = (g_banner_blink & 1) ? theme : LCD_WHITE;
        fg = (g_banner_blink & 1) ? LCD_WHITE : theme;
    } else {
        g_banner_blink = 0;
    }

    lcd_fill(6, 52, LCD_W - 7, 78, bg);
    lcd_draw_rectangle(6, 52, LCD_W - 7, 78, theme);
    lcd_show_mixed_text(12, 57, demo_stage_text(data->charge_state), fg, bg, 16, 0);

    snprintf(risk_text, sizeof(risk_text), "Risk:%u", data->risk_score);
    lcd_show_string(242, 58, (const uint8_t *)risk_text, fg, bg, 16, 0);
}

static void lcd_show_flow_bar(charge_state_t state)
{
    static const uint16_t node_x[5] = {28, 92, 156, 220, 284};
    static const char *labels[5] = {"待机", "充电", "告警", "断电", "复位"};
    uint8_t step = demo_flow_step(state);
    uint16_t theme = demo_theme_color(state);

    lcd_fill(0, 82, LCD_W - 1, 124, LCD_WHITE);

    for (uint8_t i = 0; i < 4; i++) {
        uint16_t color = (i + 1 < step) ? theme : LCD_LGRAY;
        lcd_draw_line(node_x[i] + 9, 96, node_x[i + 1] - 9, 96, color);
        lcd_draw_line(node_x[i] + 9, 97, node_x[i + 1] - 9, 97, color);
    }

    for (uint8_t i = 0; i < 5; i++) {
        uint16_t color = (i + 1 <= step) ? theme : LCD_LGRAY;
        lcd_fill(node_x[i] - 5, 91, node_x[i] + 5, 101, color);
        lcd_draw_rectangle(node_x[i] - 7, 89, node_x[i] + 7, 103,
            (i + 1 == step) ? theme : LCD_GRAY);
        lcd_show_mixed_text(node_x[i] - 12, 108, labels[i], color, LCD_WHITE, 12, 0);
    }
}

static void lcd_show_metric_tile(uint16_t x, uint16_t y, const char *label,
    const char *value, uint16_t value_color)
{
    lcd_fill(x, y, x + 146, y + 27, LCD_WHITE);
    lcd_draw_rectangle(x, y, x + 146, y + 27, LCD_LGRAY);
    lcd_show_mixed_text(x + 6, y + 5, label, LCD_DARKBLUE, LCD_WHITE, 12, 0);
    lcd_show_mixed_text(x + 56, y + 5, value, value_color, LCD_WHITE, 16, 0);
}

static void lcd_show_demo_metrics(const charge_safety_data_t *data)
{
    char temp_text[20] = {0};
    char curr_text[20] = {0};
    char smoke_text[20] = {0};
    const char *output_text = data->relay_state ? "通" : "断";
    const char *cooling_text = charge_feedback_cooling_active() ? "开" : "关";
    const char *human_text = data->human_present ? "有" : "无";

    snprintf(temp_text, sizeof(temp_text), "%.1fC", data->temperature_c);
    snprintf(curr_text, sizeof(curr_text), "%.2fA", data->power.current_a);
    snprintf(smoke_text, sizeof(smoke_text), "%.0f", data->smoke_level);

    lcd_fill(0, 126, LCD_W - 1, 210, LCD_WHITE);
    lcd_show_metric_tile(8, 126, "温度", temp_text,
        data->temperature_c >= 45.0f ? LCD_RED : LCD_BLACK);
    lcd_show_metric_tile(166, 126, "电流", curr_text,
        data->power.current_a >= 0.8f ? LCD_RED : LCD_BLACK);
    lcd_show_metric_tile(8, 154, "烟雾", smoke_text,
        data->smoke_level >= 35.0f ? LCD_RED : LCD_BLACK);
    lcd_show_metric_tile(166, 154, "输出", output_text,
        data->relay_state ? LCD_GREEN : LCD_RED);
    lcd_show_metric_tile(8, 182, "散热", cooling_text,
        charge_feedback_cooling_active() ? LCD_BLUE : LCD_BLACK);
    lcd_show_metric_tile(166, 182, "人体", human_text,
        data->human_present ? LCD_MAGENTA : LCD_BLACK);
}

void lcd_dev_init(void)
{
    lcd_init();
    lcd_fill(0, 0, LCD_W - 1, LCD_H - 1, LCD_WHITE);
}

void lcd_show_ui(void)
{
    charge_safety_data_t data;
    uint16_t key_bg = LCD_WHITE;
    uint16_t key_fg = LCD_BLUE;
    const char *network_text = "OFF";
    uint16_t network_color = LCD_RED;

    charge_safety_get_data(&data);

    if (!g_ui_initialized) {
        lcd_fill(0, 0, LCD_W - 1, LCD_H - 1, LCD_WHITE);
        g_ui_initialized = true;
    }

    lcd_show_demo_title();
    lcd_show_picture(216, 0, IMG_SC_CARD_W, IMG_SC_CARD_H,
        safe_charge_status_card(data.charge_state));
    if (g_network_state) {
        network_text = "NET";
        network_color = LCD_BLUE;
    } else if (iot_connection_requested()) {
        network_text = "LINK";
        network_color = LCD_BROWN;
    }
    lcd_fill(184, 34, 214, 48, LCD_WHITE);
    lcd_show_string(184, 34, (const uint8_t *)network_text,
        network_color, LCD_WHITE, 12, 0);

    lcd_show_stage_panel(&data);
    lcd_show_flow_bar(data.charge_state);
    lcd_show_demo_metrics(&data);

    lcd_fill(0, 214, LCD_W - 1, LCD_H - 1, LCD_WHITE);
    lcd_show_mixed_text(8, 218, "上:授权 下:复位 左:告警 右:断电",
        LCD_BLUE, LCD_WHITE, 12, 0);
    char footer_text[32] = {0};
    snprintf(footer_text, sizeof(footer_text), " A:%s EV:%lu",
        data.access_authorized ? "OK" : "NO", (unsigned long)data.event_count);
    lcd_show_string(218, 218, (const uint8_t *)footer_text,
        data.access_authorized ? LCD_GREEN : LCD_RED, LCD_WHITE, 12, 0);

    if (g_key_flash_ticks > 0) {
        key_bg = g_key_flash_color;
        key_fg = LCD_WHITE;
        g_key_flash_ticks--;
    }
    lcd_fill(0, 214, LCD_W - 1, LCD_H - 1, key_bg);
    lcd_show_string(8, 216, (const uint8_t *)g_last_key_text, key_fg, key_bg, 16, 0);
    lcd_show_string(154, 218, (const uint8_t *)"K3 START K6 CUT K4 RST",
        key_fg, key_bg, 12, 0);
    lcd_show_string(242, 232, (const uint8_t *)footer_text, key_fg, key_bg, 12, 0);
}

void lcd_set_temperature(double temperature)
{
    (void)temperature;
}

void lcd_set_humidity(double humidity)
{
    (void)humidity;
}

void lcd_set_illumination(double illumination)
{
    (void)illumination;
}

void lcd_set_light_state(bool state)
{
    light_set_state(state);
}

void lcd_set_motor_state(bool state)
{
    charge_safety_handle_command(state ? CHARGE_CMD_ACCESS_AUTH_OK : CHARGE_CMD_STOP);
}

void lcd_set_auto_state(bool state)
{
    (void)state;
}

void lcd_set_network_state(int state)
{
    g_network_state = (state != 0);
    charge_safety_set_network_state(g_network_state);
}

static bool smart_home_is_protection_locked(void)
{
    charge_safety_data_t data;

    charge_safety_get_data(&data);
    return (data.charge_state == CHARGE_STATE_CUT_OFF ||
        data.charge_state == CHARGE_STATE_FAULT);
}

static void smart_home_mark_key(int key_no, const char *action, uint16_t color)
{
    const char *key_name = "KEY";

    switch (key_no) {
        case KEY_UP:
            key_name = "K3";
            break;
        case KEY_DOWN:
            key_name = "K4";
            break;
        case KEY_LEFT:
            key_name = "K5";
            break;
        case KEY_RIGHT:
            key_name = "K6";
            break;
        default:
            break;
    }

    snprintf(g_last_key_text, sizeof(g_last_key_text), "KEY %s %s", key_name, action);
    g_key_flash_color = color;
    g_key_flash_ticks = 8;
    printf("[safe_charge] %s\n", g_last_key_text);
}

static void smart_home_start_charge_now(int key_no)
{
    charge_safety_data_t data;

    if (smart_home_is_protection_locked()) {
        smart_home_mark_key(key_no, "LOCKED", LCD_RED);
        lcd_show_ui();
        return;
    }

    smart_home_mark_key(key_no, "START NET", LCD_GREEN);
    iot_request_connect();
    charge_safety_handle_command(CHARGE_CMD_ACCESS_AUTH_OK);
    charge_safety_get_data(&data);
    charge_feedback_apply(&data);
    lcd_show_ui();
}

static void smart_home_reset_now(int key_no)
{
    charge_safety_data_t data;

    smart_home_mark_key(key_no, "RESET", LCD_BLUE);
    charge_feedback_reset_idle();
    charge_safety_handle_command(CHARGE_CMD_RESET);
    charge_feedback_reset_idle();
    charge_safety_get_data(&data);
    charge_feedback_apply(&data);
    lcd_show_ui();
}

static void smart_home_cutoff_now(int key_no)
{
    charge_safety_data_t data;

    smart_home_mark_key(key_no, "CUT OFF", LCD_RED);
    charge_feedback_emergency_cutoff();
    charge_safety_handle_command(CHARGE_CMD_RELAY_OFF);
    charge_safety_get_data(&data);
    charge_feedback_apply(&data);
    lcd_show_ui();
}

void smart_home_key_fast_action(int key_no)
{
    switch (key_no) {
        case KEY_UP:
            smart_home_start_charge_now(key_no);
            break;
        case KEY_DOWN:
            smart_home_reset_now(key_no);
            break;
        case KEY_LEFT:
            smart_home_cutoff_now(key_no);
            break;
        case KEY_RIGHT:
            smart_home_cutoff_now(key_no);
            break;
        default:
            break;
    }
}

void smart_home_key_process(int key_no)
{
    printf("[safe_charge] key=%d\n", key_no);
    smart_home_key_fast_action(key_no);
}

void smart_home_iot_cmd_process(int iot_cmd)
{
    switch (iot_cmd) {
        case IOT_CMD_LIGHT_ON:
            light_set_state(true);
            break;
        case IOT_CMD_LIGHT_OFF:
            light_set_state(false);
            break;
        case IOT_CMD_MOTOR_ON:
            charge_safety_handle_command(CHARGE_CMD_ACCESS_AUTH_OK);
            break;
        case IOT_CMD_MOTOR_OFF:
            charge_safety_handle_command(CHARGE_CMD_STOP);
            break;
        case IOT_CMD_CHARGE_START:
            charge_safety_handle_command(CHARGE_CMD_ACCESS_AUTH_OK);
            break;
        case IOT_CMD_CHARGE_STOP:
            charge_safety_handle_command(CHARGE_CMD_STOP);
            break;
        case IOT_CMD_CHARGE_RESET:
            charge_feedback_reset_idle();
            charge_safety_handle_command(CHARGE_CMD_RESET);
            break;
        case IOT_CMD_CHARGE_CUTOFF:
            charge_feedback_emergency_cutoff();
            charge_safety_handle_command(CHARGE_CMD_RELAY_OFF);
            break;
        case IOT_CMD_CHARGE_SIM_SMOKE:
            charge_safety_handle_command(CHARGE_CMD_SIM_SMOKE);
            break;
        case IOT_CMD_CHARGE_CLEAR_SMOKE:
            charge_safety_handle_command(CHARGE_CMD_CLEAR_SMOKE);
            break;
        case IOT_CMD_CHARGE_SIM_FULL:
            charge_safety_handle_command(CHARGE_CMD_SIM_FULL);
            break;
        case IOT_CMD_CHARGE_SIM_TEMP_WARN:
            charge_safety_handle_command(CHARGE_CMD_SIM_TEMP_WARN);
            break;
        case IOT_CMD_CHARGE_SIM_CURRENT_WARN:
            charge_safety_handle_command(CHARGE_CMD_SIM_CURRENT_WARN);
            break;
        case IOT_CMD_CHARGE_SIM_GAS_WARN:
            charge_safety_handle_command(CHARGE_CMD_SIM_GAS_WARN);
            break;
        case IOT_CMD_CHARGE_DEMO_NEXT:
            charge_safety_handle_command(CHARGE_CMD_DEMO_NEXT);
            break;
        case IOT_CMD_CHARGE_AUTH_OK:
            charge_safety_handle_command(CHARGE_CMD_ACCESS_AUTH_OK);
            break;
        case IOT_CMD_CHARGE_AUTH_DENY:
            charge_safety_handle_command(CHARGE_CMD_ACCESS_AUTH_DENY);
            break;
        default:
            break;
    }
}

void smart_home_su03t_cmd_process(int su03t_cmd)
{
    switch (su03t_cmd) {
        case light_state_on:
            light_set_state(true);
            break;
        case light_state_off:
            light_set_state(false);
            break;
        case motor_state_on:
            charge_safety_handle_command(CHARGE_CMD_ACCESS_AUTH_OK);
            break;
        case motor_state_off:
            charge_safety_handle_command(CHARGE_CMD_STOP);
            break;
        case temperature_get:
        {
            double temp = 0;
            double humi = 0;
            sht30_read_data(&temp, &humi);
            su03t_send_double_msg(1, temp);
            break;
        }
        case humidity_get:
        {
            double temp = 0;
            double humi = 0;
            sht30_read_data(&temp, &humi);
            su03t_send_double_msg(2, humi);
            break;
        }
        case illumination_get:
        {
            double lum = 0;
            bh1750_read_data(&lum);
            su03t_send_double_msg(3, lum);
            break;
        }
        case voice_charge_start:
            charge_safety_handle_command(CHARGE_CMD_ACCESS_AUTH_OK);
            break;
        case voice_charge_stop:
            charge_safety_handle_command(CHARGE_CMD_STOP);
            break;
        case voice_charge_reset:
            charge_safety_handle_command(CHARGE_CMD_RESET);
            break;
        case voice_charge_gas_warn:
            charge_safety_handle_command(CHARGE_CMD_SIM_GAS_WARN);
            break;
        case voice_charge_cutoff:
            charge_safety_handle_command(CHARGE_CMD_RELAY_OFF);
            break;
        case voice_charge_full:
            charge_safety_handle_command(CHARGE_CMD_SIM_FULL);
            break;
        case voice_charge_temp_warn:
            charge_safety_handle_command(CHARGE_CMD_SIM_TEMP_WARN);
            break;
        default:
            break;
    }
}
