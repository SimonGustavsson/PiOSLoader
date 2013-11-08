#include "gpio.h"

static void delay(unsigned int count) 
{
	volatile unsigned int i;
	for (i = 0; i < count; i++){ /* Do Nothing */ }
}

gpio_reg* gGpio;

void gpio_enable_uart(void)
{
	gGpio = (gpio_reg*)(GPIO_BASE);

	// Disable pull up/down for all GPIO pins & delay for 150 cycles.
	gGpio->gppud.raw = 0x00000000;
	delay(150);

	// Disable pull up/down for pin 14,15 & delay for 150 cycles.
	gGpio->gppudclk0.raw = (1 << 14) | (1 << 15);
	delay(150);

	// Write 0 to GPPUDCLK0 to make it take effect.
	gGpio->gppudclk0.raw = 0x00000000;

}