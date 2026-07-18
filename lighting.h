#ifndef LIGHTING_H
#define LIGHTING_H

#include "led_output.h"

Color lighting_scale_color(Color color, uint8_t brightness);

/* Scale using an inexpensive gamma-2 approximation. A value of 128 is
 * approximately half perceived brightness rather than half PWM output. */
Color lighting_scale_perceived(Color color, uint8_t perceived_brightness);
void lighting_clear(LedFrame frame);
void lighting_fill(LedFrame frame, Color color);
void lighting_set_mirrored(LedFrame frame, uint8_t index, Color color);

/* Render a mirrored red/amber/green battery bar between empty and full. */
void lighting_render_battery_bar(LedFrame frame, uint16_t millivolts,
                                 uint16_t empty_mv, uint16_t full_mv,
                                 uint8_t brightness);

#endif
