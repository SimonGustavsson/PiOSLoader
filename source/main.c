#include "uart.h"
#include "interrupt.h"

// In bytes
#define KERNEL_MAX_SIZE (1024 * 1024)

extern void enable_irq(void);

volatile unsigned int gSizeBytesReceived;
volatile unsigned int gKernelSize;
volatile unsigned int gKernelBytesReceived;
char gBuffer[KERNEL_MAX_SIZE];
volatile unsigned int gUserConnected;
unsigned int gBufferIndex;

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

	if (gSizeBytesReceived < 4)
	{
		gSizeBytesReceived++;

		if (gSizeBytesReceived == 4)
		{
			gKernelSize = (gBuffer[3] << 24) + (gBuffer[2] << 16) + (gBuffer[1] << 8) + gBuffer[0];

			if (gKernelSize > KERNEL_MAX_SIZE)
			{
				uart_puts("Kernel too big!\n\r");
			}
			else
			{
				uart_puts("Gimme dat kernel, bro!\n\r");
			}
		}

		gBufferIndex++;

		return;
	}
	else
	{
		gKernelBytesReceived++;
	}

	// We done?
	if (gKernelBytesReceived == gKernelSize)
	{
		char* dest = (char*)(0x8000);
		memcpy(dest, &gBuffer[3], gKernelSize);

		void(*start)(void) = (void(*)())(0x8000);
		start();
	}

	gBufferIndex++;
}

void cmain(void)
{
	unsigned int i;

	// Zero out the global vars, because I don't trust BSS. <Paranoia />
	gKernelBytesReceived = 0;
	gBufferIndex = 0;
	gSizeBytesReceived = 0;
	gKernelSize = 0;
	gUserConnected = 0;
	for (i = 0; i < 1024 * 1024; i++)
		gBuffer[i] = 0;

	uart_init();

	uart_irpt_enable();
	arm_interrupt_init();
	arm_irq_disableall();
	arm_irq_enable(interrupt_source_uart);
	enable_irq();

	// Wait for user to connect
	while (!gUserConnected) { /* Do Nothing */ }


	while (1);
}