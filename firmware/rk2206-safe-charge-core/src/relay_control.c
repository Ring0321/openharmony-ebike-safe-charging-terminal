#include "relay_control.h"

#include "iot_gpio.h"

#ifndef SAFE_CHARGE_RELAY_ENABLE_GPIO
#define SAFE_CHARGE_RELAY_ENABLE_GPIO 1
#endif

#ifndef SAFE_CHARGE_RELAY_GPIO
#define SAFE_CHARGE_RELAY_GPIO GPIO0_PB2
#endif

#ifndef SAFE_CHARGE_COOLING_RELAY_ENABLE_GPIO
#define SAFE_CHARGE_COOLING_RELAY_ENABLE_GPIO 1
#endif

#ifndef SAFE_CHARGE_COOLING_RELAY_GPIO
#define SAFE_CHARGE_COOLING_RELAY_GPIO GPIO0_PB1
#endif

#ifndef SAFE_CHARGE_WARNING_RELAY_ENABLE_GPIO
#define SAFE_CHARGE_WARNING_RELAY_ENABLE_GPIO 1
#endif

#ifndef SAFE_CHARGE_WARNING_RELAY_GPIO
#define SAFE_CHARGE_WARNING_RELAY_GPIO GPIO0_PA5
#endif

#ifndef SAFE_CHARGE_PROTECTION_RELAY_ENABLE_GPIO
#define SAFE_CHARGE_PROTECTION_RELAY_ENABLE_GPIO 1
#endif

#ifndef SAFE_CHARGE_PROTECTION_RELAY_GPIO
#define SAFE_CHARGE_PROTECTION_RELAY_GPIO GPIO0_PB0
#endif

#ifndef SAFE_CHARGE_RELAY_ACTIVE_LOW
#define SAFE_CHARGE_RELAY_ACTIVE_LOW 1
#endif

typedef struct {
    unsigned int gpio;
    bool enabled;
    const char *role;
} relay_channel_config_t;

static const relay_channel_config_t g_relay_configs[SAFE_CHARGE_RELAY_COUNT] = {
    [SAFE_CHARGE_RELAY_K1_OUTPUT] = {
        SAFE_CHARGE_RELAY_GPIO,
        SAFE_CHARGE_RELAY_ENABLE_GPIO,
        "CHARGE_OUTPUT",
    },
    [SAFE_CHARGE_RELAY_K2_COOLING] = {
        SAFE_CHARGE_COOLING_RELAY_GPIO,
        SAFE_CHARGE_COOLING_RELAY_ENABLE_GPIO,
        "COOLING",
    },
    [SAFE_CHARGE_RELAY_K3_WARNING] = {
        SAFE_CHARGE_WARNING_RELAY_GPIO,
        SAFE_CHARGE_WARNING_RELAY_ENABLE_GPIO,
        "ALERT_WARNING",
    },
    [SAFE_CHARGE_RELAY_K4_PROTECTION] = {
        SAFE_CHARGE_PROTECTION_RELAY_GPIO,
        SAFE_CHARGE_PROTECTION_RELAY_ENABLE_GPIO,
        "PROTECT_LOCK",
    },
};

static bool g_relay_states[SAFE_CHARGE_RELAY_COUNT] = {false};
static bool g_relay_force_cutoff = false;

static IotGpioValue relay_state_to_gpio_value(bool state)
{
#if SAFE_CHARGE_RELAY_ACTIVE_LOW
    return state ? IOT_GPIO_VALUE0 : IOT_GPIO_VALUE1;
#else
    return state ? IOT_GPIO_VALUE1 : IOT_GPIO_VALUE0;
#endif
}

void relay_control_init(void)
{
    for (unsigned int i = 0; i < SAFE_CHARGE_RELAY_COUNT; ++i) {
        g_relay_states[i] = false;
        if (!g_relay_configs[i].enabled) {
            continue;
        }
        IoTGpioInit(g_relay_configs[i].gpio);
        IoTGpioSetDir(g_relay_configs[i].gpio, IOT_GPIO_DIR_OUT);
        IoTGpioSetOutputVal(g_relay_configs[i].gpio, relay_state_to_gpio_value(false));
    }
    g_relay_force_cutoff = false;
}

void relay_set_channel_state(safe_charge_relay_channel_t channel, bool state)
{
    if (channel < 0 || channel >= SAFE_CHARGE_RELAY_COUNT) {
        return;
    }
    if (g_relay_force_cutoff && channel == SAFE_CHARGE_RELAY_K1_OUTPUT && state) {
        state = false;
    }
    g_relay_states[channel] = state;
    if (g_relay_configs[channel].enabled) {
        IoTGpioSetOutputVal(g_relay_configs[channel].gpio, relay_state_to_gpio_value(state));
    }
}

bool relay_get_channel_state(safe_charge_relay_channel_t channel)
{
    if (channel < 0 || channel >= SAFE_CHARGE_RELAY_COUNT) {
        return false;
    }
    return g_relay_states[channel];
}

const char *relay_get_channel_role(safe_charge_relay_channel_t channel)
{
    if (channel < 0 || channel >= SAFE_CHARGE_RELAY_COUNT) {
        return "UNKNOWN";
    }
    return g_relay_configs[channel].role;
}

void relay_set_state(bool state)
{
    relay_set_channel_state(SAFE_CHARGE_RELAY_K1_OUTPUT, state);
}

bool relay_get_state(void)
{
    return relay_get_channel_state(SAFE_CHARGE_RELAY_K1_OUTPUT);
}

void relay_force_cutoff(bool enabled)
{
    g_relay_force_cutoff = enabled;
    if (enabled) {
        relay_set_state(false);
    }
}

bool relay_is_force_cutoff(void)
{
    return g_relay_force_cutoff;
}

void relay_set_cooling_state(bool state)
{
    relay_set_channel_state(SAFE_CHARGE_RELAY_K2_COOLING, state);
}

bool relay_get_cooling_state(void)
{
    return relay_get_channel_state(SAFE_CHARGE_RELAY_K2_COOLING);
}

void relay_set_warning_state(bool state)
{
    relay_set_channel_state(SAFE_CHARGE_RELAY_K3_WARNING, state);
}

bool relay_get_warning_state(void)
{
    return relay_get_channel_state(SAFE_CHARGE_RELAY_K3_WARNING);
}

void relay_set_protection_state(bool state)
{
    relay_set_channel_state(SAFE_CHARGE_RELAY_K4_PROTECTION, state);
}

bool relay_get_protection_state(void)
{
    return relay_get_channel_state(SAFE_CHARGE_RELAY_K4_PROTECTION);
}

void relay_all_off(void)
{
    for (unsigned int i = 0; i < SAFE_CHARGE_RELAY_COUNT; ++i) {
        relay_set_channel_state((safe_charge_relay_channel_t)i, false);
    }
}
