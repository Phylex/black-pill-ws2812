#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/timer.h>

#define LED_DATA_PIN GPIO5
#define LED_DATA_PORT GPIOA
#define LED_BUFFER_DEPTH 6
#define BITS_PER_LED 24
#define LED_0_HIGH_PERIOD 42
#define LED_1_HIGH_PERIOD 82
#define LED_PERIOD 250

void setup_timer(void) {
	// setup gpio for the timer
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
	gpio_set_af(GPIOA, GPIO_AF1, GPIO5);

	// enable clock and do a reset of the state
	rcc_periph_clock_enable(RCC_TIM2);
	rcc_periph_reset_pulse(RST_TIM2);
	timer_enable_irq(TIM2, TIM_DIER_UIE);
	nvic_enable_irq(NVIC_TIM2_IRQ);

	// setup the counting mode and internal clock multiplication
	timer_set_mode(TIM2,
			TIM_CR1_CKD_CK_INT, // no multiplication from the AHB1 bus clock
			TIM_CR1_CMS_EDGE, // 
			TIM_CR1_DIR_UP); // count up
	timer_continuous_mode(TIM2);
	// set up the timer to produce a PWM output on all 4 channels
	timer_set_oc_mode(TIM2,
			TIM_OC1,
			TIM_OCM_PWM1);
	timer_set_prescaler(TIM2, 0);
	timer_set_period(TIM2, LED_PERIOD);
	
	// enable the preload register so that the counter val
	// is only changed at 'update events'
	timer_enable_preload(TIM2);
	timer_enable_oc_preload(TIM2, TIM_OC1);
	timer_enable_update_event(TIM2);
	
	//enable the timer interrupt
	timer_update_on_overflow(TIM2); // generate irq/dma from the under/overflow
	
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3);
}

void tim2_isr() {
	timer_clear_flag(TIM2, TIM_SR_CC1IF);
	gpio_toggle(GPIOA, GPIO3);
}

int main(void) {
	// set up the clock to run from an external crystal
	rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_84MHZ]);
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_GPIOA);
	setup_timer();
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO4);
	gpio_set(GPIOA, GPIO4);
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
	timer_enable_counter(TIM2);
	while (1) {
		for (int i = 0; i < 10000; i++) {
			__asm__("nop");
		}
		gpio_toggle(GPIOC, GPIO13);
		gpio_toggle(GPIOA, GPIO4);
	}
}
