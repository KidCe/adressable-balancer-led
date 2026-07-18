#include "lighting.h"

#include <string.h>

static uint8_t scale8(uint8_t value, uint8_t brightness)
{
    return (uint8_t)(((uint16_t)value * brightness) / 255U);
}

static uint8_t battery_bar_count(uint16_t millivolts, uint16_t empty_mv,
                                 uint16_t full_mv)
{
    if (millivolts <= empty_mv || full_mv <= empty_mv)
        return 0;
    if (millivolts >= full_mv)
        return LED_HALF;

    uint32_t range = full_mv - empty_mv;
    uint32_t above_empty = millivolts - empty_mv;
    uint32_t count = (above_empty * LED_HALF + range - 1U) / range;
    return (count > LED_HALF) ? LED_HALF : (uint8_t)count;
}

static Color battery_bar_color(uint8_t index)
{
    if (index < 2U) {
        Color red = {255, 0, 0};
        return red;
    }
    if (index < 5U) {
        Color amber = {255, 160, 0};
        return amber;
    }

    Color green = {0, 255, 0};
    return green;
}

Color lighting_scale_color(Color color, uint8_t brightness)
{
    color.r = scale8(color.r, brightness);
    color.g = scale8(color.g, brightness);
    color.b = scale8(color.b, brightness);
    return color;
}

Color lighting_scale_perceived(Color color, uint8_t perceived_brightness)
{
    uint32_t perceived = perceived_brightness;
    uint8_t linear_brightness = (uint8_t)((perceived * perceived + 254U) /
                                          255U);
    return lighting_scale_color(color, linear_brightness);
}

void lighting_clear(LedFrame frame)
{
    memset(frame, 0, sizeof(LedFrame));
}

void lighting_fill(LedFrame frame, Color color)
{
    for (uint8_t i = 0; i < LED_COUNT; i++)
        frame[i] = color;
}

void lighting_set_mirrored(LedFrame frame, uint8_t index, Color color)
{
    if (index >= LED_HALF)
        return;

    frame[index] = color;
    frame[LED_COUNT - 1U - index] = color;
}

void lighting_render_battery_bar(LedFrame frame, uint16_t millivolts,
                                 uint16_t empty_mv, uint16_t full_mv,
                                 uint8_t brightness)
{
    uint8_t count = battery_bar_count(millivolts, empty_mv, full_mv);

    lighting_clear(frame);
    for (uint8_t i = 0; i < count; i++) {
        Color color = lighting_scale_color(battery_bar_color(i), brightness);
        lighting_set_mirrored(frame, i, color);
    }
}
