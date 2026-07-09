#ifndef _DRV_LIGHT_H__
#define _DRV_LIGHT_H__

#include  "stdbool.h"

typedef enum {
    LIGHT_COLOR_OFF = 0,
    LIGHT_COLOR_RED,
    LIGHT_COLOR_GREEN,
    LIGHT_COLOR_BLUE,
    LIGHT_COLOR_ORANGE,
    LIGHT_COLOR_WHITE,
} light_color_t;

void light_dev_init(void);

void light_set_rgb(bool red, bool green, bool blue);

void light_set_color(light_color_t color);

void light_set_state(bool state);

int get_light_state(void);

light_color_t get_light_color(void);

const char *light_color_to_string(light_color_t color);

#endif
