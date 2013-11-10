#include "uart.h"
#include "interrupt.h"

// In bytes
#define KERNEL_MAX_SIZE 512000 // (Bytes)

extern void enable_irq(void);

volatile unsigned int gKernelSize;
char gBuffer[KERNEL_MAX_SIZE];
unsigned int gBufferIndex;
volatile unsigned int gKernelReceived;

static inline void *memcpy(void *dest, const void *src, unsigned int bytesToCopy)
{
	char *s = (char *)src;
	char *d = (char *)dest;
	while (bytesToCopy > 0)
	{
		*d++ = *s++;
		bytesToCopy--;
	}
	return dest;
}

void c_irq_handler(void)
{
	gBuffer[gBufferIndex] = uart_getc();

	// Did we just receive the last size byte?
	if (gBufferIndex == 3)
	{
		gKernelSize = (gBuffer[3] << 24) + (gBuffer[2] << 16) + (gBuffer[1] << 8) + gBuffer[0];

		if(gKernelSize < KERNEL_MAX_SIZE)
			uart_puts("SIZEOK");
		else
			uart_puts("BADSIZE");
	}

	if(gBufferIndex - 3 == gKernelSize) // Size is 4, 0 indexed
	{
		gKernelReceived = 1;
	}

	gBufferIndex++;
}

void cmain(void)
{
	// Zero out the global vars, because I don't trust BSS. <Paranoia />
	unsigned int i;
	gBufferIndex = 0;
	gKernelSize = 0;
	gKernelReceived = 0;
	
	for (i = 0; i < KERNEL_MAX_SIZE; i++) { gBuffer[i] = 0; }

	uart_init();

	uart_irpt_enable();
	arm_interrupt_init();
	arm_irq_disableall();
	arm_irq_enable(interrupt_source_uart);
	enable_irq();

	uart_puts("PiOS Loader started, waiting for kernel.\n");

	// Wait for user to connect
	while (!gKernelReceived) { /* Wait */ }
	
	uart_puts("KRNOK");

	// Move kernel to 0x8000
	char* dest = (char*)(0x8000);
	memcpy(dest, &gBuffer[4], gKernelSize + 1);

	// Jump into the new shiny kernel
	void(*start)(void) = (void(*)())(0x8000);
	start();

	while (1);
}