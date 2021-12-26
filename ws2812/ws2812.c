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
#include "libws2812.h"

static inline void clock_setup(void);

WS2812Strip *Tim2_strip =  (WS2812Strip *)0;

static inline void clock_setup(void)
{
	rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);
}

int main(void)
{
	clock_setup();
	WS2812Strip strip;
	float start = 0;
	float step = 360. / LED_STRIP_LENGTH;
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
		start = fmodf((start + 1./5.), LED_STRIP_LENGTH);
		for (int j = 0; j < LED_STRIP_LENGTH; j++) {
			c = hue_to_rgb((j + start) * step);
			strip.buffer[j].encoded_color = init_pixel_col(c.red, c.green, c.blue);
		}
		for (int k = 0; k < 2000000; k++) {
			__asm__("nop");
		}
	}
	return 0;
}
