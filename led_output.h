#ifndef LED_OUTPUT_H
#define LED_OUTPUT_H

#include <stdint.h>

#define LED_COUNT 14U
#define LED_HALF  7U

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;

typedef Color LedFrame[LED_COUNT];

typedef enum {
    LED_OUTPUT_REPEAT_FRAME,
    LED_OUTPUT_FRAME_THEN_OFF
} LedOutputMode;

/* Configure the LED data pin and SPI peripheral. */
void led_output_init(void);

/* Transmit one frame. The frame data is copied synchronously before return. */
void led_output_show(const LedFrame frame, LedOutputMode mode);

#endif
