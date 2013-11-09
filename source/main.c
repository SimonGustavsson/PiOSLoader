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
volatile unsigned int gWakeupReceived;

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
	// First accept a random byte to establish connection before receiving kernel size
	if (!gWakeupReceived)
	{
		gWakeupReceived = 1;
		uart_getc();
		return;
	}

	gBuffer[gBufferIndex] = uart_getc();

	// Are we still receiving the size of the kernel?
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
				uart_putc(gBuffer[0]);
				uart_putc(gBuffer[1]);
				uart_putc(gBuffer[2]);
				uart_putc(gBuffer[3]);
			}
		}

		gBufferIndex++;

		return;
	}

	// We're receiving the kernel
	gKernelBytesReceived++;

	// We done?
	if (gKernelBytesReceived == gKernelSize)
	{
		// TODO: This doesn't work with large kernels currently as we overwrite the current instruction!
		// TODO: Relocate the bootloader kernel to an address less than 8000 prior to receiving the kernel over serial
		char* dest = (char*)(0x8000);
		memcpy(dest, &gBuffer[4], gKernelSize);

		// DEBUG: Print the first 20 bytes of the kernel back to the deployer after the move
		// So that it can verify that everything went fine
		unsigned int i;
		for (i = 0; i < 20; i++)
			uart_putc(dest[i]);

		// Jump into the new shiny kernel
		void(*start)(void) = (void(*)())(0x8000);
		start();
	}

	gBufferIndex++;
}

void cmain(void)
{
	unsigned int i;

	// Zero out the global vars, because I don't trust BSS. <Paranoia />
	gWakeupReceived = 0;
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