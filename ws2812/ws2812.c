/*
 * Copyright (C) 2015 Piotr Esden-Tempski <piotr@esden.net>
 * Copyright (C) 2015 Jack Ziesing <jziesing@gmail.com>
 *
 * Extensively modified to work with the WS2812 pixels
 * Alexander Becker
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <math.h>

#ifndef ARRAY_LEN
#define ARRAY_LEN(array) (sizeof((array))/sizeof((array)[0]))
#endif


#define LED_DATA_PIN GPIO5
#define LED_DATA_PORT GPIOA
#define LED_STRIP_LENGTH 50
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
static void clock_setup(void);
static void gpio_setup(void);
static void tim_setup(void);
void setup_ws2812_strip(WS2812Strip *);
void display_strip(void);
uint32_t get_bit_value(WS2812Strip *strip);
Color hue_to_rgb(float);


WS2812Strip *Tim2_strip =  (WS2812Strip *)0;

uint32_t get_bit_value(WS2812Strip *strip) {
		if (strip->current_pixel->current_bit == BITS_PER_LED) {
			strip->current_pixel->current_bit = 0;
			++strip->current_pixel;
		}
		if (strip->current_pixel == strip->end_of_strip) {
			strip->current_pixel = &Tim2_strip->buffer[0];
			return 0;
		}
		uint32_t bit_value = (strip->current_pixel->encoded_color >>
				(23 - strip->current_pixel->current_bit)) & 0x01;
		++strip->current_pixel->current_bit;
		// assuming the pixel and bit are valid set the register accordingly
		if (bit_value == 1) {
			return LED_1_HIGH_PERIOD;
		} else {
			return LED_0_HIGH_PERIOD;
		}
}

uint32_t init_pixel_col(uint8_t r, uint8_t g, uint8_t b) {
	return ((uint32_t)g << 16) |
		   ((uint32_t)r << 8 ) |
		    (uint32_t) b;
}

static void clock_setup(void)
{
	rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);
}

static void gpio_setup(void)
{
	/* Enable GPIO clock for leds. */
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_GPIOA);

	/* Enable led as output */
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
	gpio_clear(GPIOC, GPIO13);
	/* Enable the timer 2 output on pin A5 */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO5);
	gpio_set_af(GPIOA, GPIO_AF1, GPIO5);
}

static void tim_setup(void)
{
	/* Enable TIM2 clock. */
	rcc_periph_clock_enable(RCC_TIM2);

	/* Enable TIM2 interrupt. */
	nvic_enable_irq(NVIC_TIM2_IRQ);

	/* Reset TIM2 peripheral to defaults. */
	rcc_periph_reset_pulse(RST_TIM2);

	/* Timer global mode:
	 * - No divider
	 * - Alignment edge
	 * - Direction up
	 */
	timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT,
		TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

	/* enable the output compare functionality */
	timer_enable_oc_output(TIM2, TIM_OC1);

	/* set the Channel one of the timer to output pwm */
	timer_set_oc_mode(TIM2,
			TIM_OC1,
			TIM_OCM_PWM1);

	/*
	 * Please take note that the clock source for STM32 timers
	 * might not be the raw APB1/APB2 clocks.  In various conditions they
	 * are doubled.  See the Reference Manual for full details!
	 * In our case, TIM2 on APB1 is running at double frequency, so this
	 * sets the prescaler to have the timer run at 5kHz
	 */
	timer_set_prescaler(TIM2, 0);

	/* Disable preload. */
	timer_disable_preload(TIM2);

	/* set the timer to run continuously */
	timer_continuous_mode(TIM2);

	/* set the PWM Duty cycle */
	timer_set_period(TIM2, LED_BIT_PERIOD);

	/* Set the initual output compare value for OC1. */
	timer_set_oc_value(TIM2, TIM_OC1, 0);

	/* Enable Channel 1 compare interrupt to recalculate compare values */
	timer_enable_irq(TIM2, TIM_DIER_UIE);
}

void tim2_isr(void)
{
	if (timer_get_flag(TIM2, TIM_SR_UIF)) {

		/* Clear compare interrupt flag. */
		timer_clear_flag(TIM2, TIM_SR_UIF);
		// set the timer to the new value
		uint32_t timer_val = get_bit_value(Tim2_strip);
		if (timer_val > 0) {
			timer_set_oc_value(TIM2, TIM_OC1, timer_val);
		} else {
			timer_set_oc_value(TIM2, TIM_OC1, 0);
			timer_disable_counter(TIM2);
			timer_set_counter(TIM2, LED_BIT_PERIOD);
		}
	}
}

void display_strip() {
	timer_enable_counter(TIM2);
}

void setup_ws2812_strip(WS2812Strip *strip) {
	strip->current_pixel = &strip->buffer[0];
	strip->end_of_strip = &strip->buffer[LED_STRIP_LENGTH];
	Tim2_strip = strip;
	tim_setup();
}

float fmodf(float a, float b) {
	float div = a / b;
	float remainder = a - ((int) div * b);
	return remainder;
}

//float fabsf(float a) {
//	if (a < 0){
//		return a * -1.;
//	}
//	return a;
//}

Color hue_to_rgb(float hue){
	hue = fmodf(hue, 360.);
	float saturation = 1.;
	float lightness = .5;
	float C = (1. - fabsf(2. * lightness - 1.)) * saturation;
	float X = C * (1. - fabsf((float)fmodf((hue / 60.), 2.) - 1.));
	float m = lightness - C / 2.;
	Color c;
	if (hue >= 0 && hue < 60) {
		c.red = (int)((C + m) * 255);
		c.green = (int)((X + m) * 255);
		c.blue = 0;
	} else if (hue >= 60 && hue < 120) {
		c.red = (int)((X + m) * 255);
		c.green = (int)((C + m) * 255);
		c.blue = 0;
	} else if (hue >= 120 && hue < 180) {
		c.red = 0;
		c.green = (int)((C + m) * 255);
		c.blue = (int)((X + m) * 255);
	} else if (hue >= 180 && hue < 240) {
		c.red = 0;
		c.green = (int)((X + m) * 255);
		c.blue = (int)((C + m) * 255);
	} else if (hue >= 240 && hue < 300) {
		c.red  = (int)((X + m) * 255);
		c.green = 0;
		c.blue = (int)((C + m) * 255);
	} else if (hue >= 300 && hue < 360){
		c.red = (int)((C + m) * 255);
		c.green = 0;
		c.blue = (int)((X + m) * 255);
	}
	return c;
}

int main(void)
{
	clock_setup();
	gpio_setup();
	WS2812Strip strip;
	float start = 0;
	float step = 360. / 50.;
	Color c;
	for (int i = 0; i < LED_STRIP_LENGTH; i++){
		c = hue_to_rgb((i + start) * step);
		WS2812Pixel pixel = {init_pixel_col(c.red, c.green, c.blue), 0};
		strip.buffer[i] = pixel;
	}
	setup_ws2812_strip(&strip);
	
	/* a line that shows that the controller is running */
	while (1) {
		display_strip();
		gpio_toggle(GPIOC, GPIO13);
		start = fmodf((start + 1), 50.);
		for (int j = 0; j < LED_STRIP_LENGTH; j++) {
			c = hue_to_rgb((j + start) * step);
			strip.buffer[j].encoded_color = init_pixel_col(c.red, c.green, c.blue);
		}
		for (int k = 0; k < 10000000; k++) {
			__asm__("nop");
		}
	}
	return 0;
}
