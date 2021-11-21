# WS2812 Example for the 'Black Pill' STM32F411 board

This is code for that can control a large amount of WS2812 that uses the Timer in PWM mode to
generate the output waveforms. Due to the high frequency of the Data transmission the STM is essentially constantly running the ISR during the
transmission. It keeps it nice and separate from the main thread of execution though.
