#include "uart.h"#include "interrupt.h"extern void enable_irq(void);volatile unsigned int gUserConnected;void c_irq_handler(void)
{
	// The only IRQ that will ever get fired is the UART - Data received
}

void cmain(void)
{
	gUserConnected = 0;

	uart_init();	uart_irpt_enable();	arm_interrupt_init();	arm_irq_disableall();	arm_irq_enable(interrupt_source_uart);	enable_irq();

	// Wait for user to connect
	while (!gUserConnected) { /* Do Nothing */ }


	while (1);
}