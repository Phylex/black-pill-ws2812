#ifndef LIBWS2812
#define LIBWS2812

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <math.h>

#define LED_DATA_PIN GPIO5
#define LED_DATA_PORT GPIOA
#define LED_STRIP_LENGTH 100
#define BITS_PER_LED 24
#define LED_0_HIGH_PERIOD 40
#define LED_1_HIGH_PERIOD 80
#define LED_BIT_PERIOD 200

typedef struct Color {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} Color;

typedef struct WS2812Pixel {
	uint32_t encoded_color;
	uint8_t current_bit;
} WS2812Pixel;

typedef struct WS2812Strip {
	WS2812Pixel buffer[LED_STRIP_LENGTH];
	WS2812Pixel *current_pixel;
	WS2812Pixel *end_of_strip;
} WS2812Strip;

uint32_t init_pixel_col(uint8_t, uint8_t, uint8_t);
void setup_ws2812_strip(WS2812Strip *);
void display_strip(void);
Color hue_to_rgb(float);
#endif
