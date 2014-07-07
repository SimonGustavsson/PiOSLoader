#include "uart.h"
#include "interrupt.h"
#include "memory.h"
#include "elf.h"
#include "utility.h"

// In bytes

extern void enable_irq(void);

volatile unsigned int gKernelSize;
char gBuffer[KERNEL_MAX_SIZE];
unsigned int gBufferIndex;
volatile unsigned int gKernelReceived;

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

void copy_func_info(char* dest, func_info* info, unsigned int info_len)
{
    // First things first, write the number of functions, null terminated integer
    *dest++ = (info_len >> 24) & 0xFF;
    *dest++ = (info_len >> 16) & 0xFF;
    *dest++ = (info_len >> 8) & 0xFF;
    *dest++ = (info_len) & 0xFF;
    *dest++ = 0;

    unsigned int i;
    for (i = 0; i < info_len; i++)
    {
        func_info* cur = &info[i];
        
        // Copy null character as well
        int nameLen = my_strlen(cur->name);
        memcpy(dest, cur->name, nameLen);

        dest[nameLen] = 0;
        dest += nameLen + 1;

        *dest++ = (cur->address >> 24) & 0xFF;
        *dest++ = (cur->address >> 16) & 0xFF;
        *dest++ = (cur->address >> 8) & 0xFF;
        *dest++ = (cur->address >> 0) & 0xFF;
    }
}

void cmain(int r0, int machineType, int atagsPa)
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

    Pallocator_Initialize();

	// Tell the deployer we want a kernel
	uart_puts("STRT");

	// Wait for user to connect
	while (!gKernelReceived) { /* Wait */ }
	
	uart_puts("KRNOK");

    int* testAlloc = (int*)palloc(4);

    char testBuf[9];
    itoa((int)testAlloc, testBuf);

    // Note: Size of kernel is in the first 4 bytes of the buffer, so skip those
    func_info* funcs;
    int funcsLen = elf_get_func_info(&gBuffer[4], gKernelSize, &funcs);

    int elfEnd = 0;
    if (funcsLen > 0)
    {
        elfEnd = elf_load(&gBuffer[4], gKernelSize);

        char* blob = (char*)(elfEnd);

        copy_func_info(blob, funcs, funcsLen);
    }
    else
    {
        uart_puts("Invalid ELF\n");
    }

    uart_puts("Bootloader: Successfully loaded kernel, booting...\n");

    // Jump into the new shiny kernel
	void(*start)(int, int, int) = (void(*)(int, int, int))(0x8000);
    start(machineType, atagsPa, elfEnd);

    uart_puts("\n\nBootloader halting * * *\n\n");

	while (1);
}