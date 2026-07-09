#include "drv_light.h"
#include "iot_gpio.h"

#define LED_R_GPIO_HANDLE GPIO0_PB5
#define LED_G_GPIO_HANDLE GPIO0_PB4
#define LED_B_GPIO_HANDLE GPIO1_PD0

static bool g_light_state = false;
static light_color_t g_light_color = LIGHT_COLOR_OFF;

void light_dev_init(void)
{
    IoTGpioInit(LED_R_GPIO_HANDLE);
    IoTGpioInit(LED_G_GPIO_HANDLE);
    IoTGpioInit(LED_B_GPIO_HANDLE);
    IoTGpioSetDir(LED_R_GPIO_HANDLE, IOT_GPIO_DIR_OUT);
    IoTGpioSetDir(LED_G_GPIO_HANDLE, IOT_GPIO_DIR_OUT);
    IoTGpioSetDir(LED_B_GPIO_HANDLE, IOT_GPIO_DIR_OUT);
    light_set_color(LIGHT_COLOR_OFF);
}

void light_set_rgb(bool red, bool green, bool blue)
{
    IoTGpioSetOutputVal(LED_R_GPIO_HANDLE, red ? IOT_GPIO_VALUE1 : IOT_GPIO_VALUE0);
    IoTGpioSetOutputVal(LED_G_GPIO_HANDLE, green ? IOT_GPIO_VALUE1 : IOT_GPIO_VALUE0);
    IoTGpioSetOutputVal(LED_B_GPIO_HANDLE, blue ? IOT_GPIO_VALUE1 : IOT_GPIO_VALUE0);
    g_light_state = red || green || blue;
}

void light_set_color(light_color_t color)
{
    if (color == g_light_color) {
        return;
    }

    switch (color) {
        case LIGHT_COLOR_RED:
            light_set_rgb(true, false, false);
            break;
        case LIGHT_COLOR_GREEN:
            light_set_rgb(false, true, false);
            break;
        case LIGHT_COLOR_BLUE:
            light_set_rgb(false, false, true);
            break;
        case LIGHT_COLOR_ORANGE:
            light_set_rgb(true, true, false);
            break;
        case LIGHT_COLOR_WHITE:
            light_set_rgb(true, true, true);
            break;
        case LIGHT_COLOR_OFF:
        default:
            light_set_rgb(false, false, false);
            break;
    }
    g_light_color = color;
}

void light_set_state(bool state)
{
    light_set_color(state ? LIGHT_COLOR_WHITE : LIGHT_COLOR_OFF);
}

int get_light_state(void)
{
    return g_light_state;
}

light_color_t get_light_color(void)
{
    return g_light_color;
}

const char *light_color_to_string(light_color_t color)
{
    switch (color) {
        case LIGHT_COLOR_RED:
            return "RED";
        case LIGHT_COLOR_GREEN:
            return "GREEN";
        case LIGHT_COLOR_BLUE:
            return "BLUE";
        case LIGHT_COLOR_ORANGE:
            return "ORANGE";
        case LIGHT_COLOR_WHITE:
            return "WHITE";
        case LIGHT_COLOR_OFF:
        default:
            return "OFF";
    }
}
