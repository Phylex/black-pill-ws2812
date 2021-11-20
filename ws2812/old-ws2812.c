#include "api.h"
#include "api-asm.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <stddef.h>
#include <libopencm3/cm3/nvic.h>

#define LED_DATA_PIN GPIO5
#define LED_DATA_PORT GPIOA
#define LED_BUFFER_DEPTH 6
#define BITS_PER_LED 24
#define LED_0_HIGH_PERIOD 42
#define LED_1_HIGH_PERIOD 82
#define LED_BIT_PERIOD 124

typedef struct WS2812Pixel {
	uint32_t encoded_color;
	uint8_t NextBit;
} WS2812Pixel;

typedef struct WS2812Strip {
	WS2812Pixel buffer[LED_BUFFER_DEPTH];
	WS2812Pixel *read_position;
	WS2812Pixel *end_of_strip;
} WS2812Strip;

WS2812Strip *Tim2_strip =  (WS2812Strip *)0;

uint32_t init_pixel_col(uint8_t r, uint8_t g, uint8_t b) {
	return ((uint32_t)g << 16) |
		   ((uint32_t)r << 8 ) |
		    (uint32_t) b;
}

void tim2_isr() {
	// check if there is a valid strip defined
	if (Tim2_strip == (WS2812Strip*)0) return;
	WS2812Pixel *current_pixel = Tim2_strip->read_position;
	// check if the next bit is still part of the current pixel
	// if not move to the next pixel
	if (current_pixel->NextBit == BITS_PER_LED) {
		current_pixel->NextBit = 0;
		++current_pixel;
		// if the next pixel is the end of the frame stop after the frame
		if (current_pixel == Tim2_strip->end_of_strip) {
			timer_disable_counter(TIM2);
			return;
		}
	}
	// assuming the pixel and bit are valid set the register accordingly
	if (((current_pixel->encoded_color >> (24 - current_pixel->NextBit)) & 1) == 1) {
		timer_set_oc_value(TIM2, TIM_OC1, LED_1_HIGH_PERIOD);
	} else {
		timer_set_oc_value(TIM2, TIM_OC1, LED_0_HIGH_PERIOD);
	}
	++current_pixel->NextBit;
}

void display_strip() {
	timer_enable_counter(TIM2);
}

void setup_ws2812_strip(WS2812Strip *strip) {
	// enable the proper output pin
	Tim2_strip = strip;
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
	gpio_set_output_options(LED_DATA_PORT,
						   GPIO_OTYPE_PP,
						   GPIO_OSPEED_50MHZ,
						   LED_DATA_PIN);
	// enable the timer and set the mode
	rcc_periph_clock_enable(RCC_TIM2);
	nvic_enable_irq(NVIC_TIM2_IRQ);
	rcc_periph_reset_pulse(RST_TIM2);
	timer_set_mode(TIM2,
				   TIM_CR1_CKD_CK_INT,
				   TIM_CR1_CMS_EDGE,
				   TIM_CR1_DIR_UP);
	timer_disable_preload(TIM2);
	timer_continuous_mode(TIM2);
	timer_set_oc_mode(TIM2,
					  TIM_OC1,
					  TIM_OCM_PWM1);
	timer_set_period(TIM2, LED_BIT_PERIOD);
	
	// enable the interrupt on overflow
	timer_enable_irq(TIM2, TIM_DIER_UIE);
}

int main(void) {
	rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);
	int i;
	WS2812Strip strip;
	WS2812Pixel pixel = {init_pixel_col(255, 0, 0), 0};
	strip.buffer[0] = pixel;
	pixel.encoded_color = init_pixel_col(127, 127, 0);
	strip.buffer[1] = pixel;
	pixel.encoded_color = init_pixel_col(0, 255, 0);
	strip.buffer[2] = pixel;
	pixel.encoded_color = init_pixel_col(0, 127, 127);
	strip.buffer[3] = pixel;
	pixel.encoded_color = init_pixel_col(0, 0, 255);
	strip.buffer[4] = pixel;
	pixel.encoded_color = init_pixel_col(127, 0, 127);
	strip.buffer[5] = pixel;
	strip.read_position = &strip.buffer[0];
	strip.end_of_strip = &strip.buffer[6];
	setup_ws2812_strip(&strip);
	display_strip();
	//rcc_periph_clock_enable(RCC_GPIOC);
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO4);
	while (1) {
		gpio_toggle(GPIOA, GPIO4);
		for (i = 0; i < 100; i++) {
			__asm__("nop");
		}
	}
}
