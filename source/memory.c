#include "memory.h"
#include <stdbool.h>
#include "uart.h"
//#include "hardware/uart.h"
//#include "util/utilities.h"
//#include "types/types.h"
//#include "types/string.h"
//#include "util/memutil.h"

unsigned char* gBitmap;
unsigned char* gMemory;

unsigned int gBytesAllocated;

void itoa(int number, char* buf)
{
    // We populate the string backwards, increment to make room for \0
    buf++;

    // Find where our string will end
    int shifter = number;
    do
    {
        buf++;
        shifter /= 10;
    } while (shifter > 0);

    // Make sure the string is terminated nicely
    *--buf = '\0';

    // Start converting the digits into characters
    do
    {
        *--buf = '0' + (number % 10); // Muahaha!
        number /= 10;
    } while (number > 0);
}


void Pallocator_Initialize(void)
{
    gBytesAllocated = 0;
    gBitmap = (unsigned char*)(0x208000);// DYN_MEM_VA_START);
    gMemory = (unsigned char*)((0x208000) + (MAX_ALLOCATED_SLICES / 8)); // (Right after the bitmap)

    // Zero out the bitmap to start with
    unsigned int i;
    int* bmpPtr = (int*)gBitmap;
    for (i = 0; i < (MAX_ALLOCATED_SLICES / 8) / 4; i++)
        *bmpPtr++ = 0;
}

static void mark_slices(unsigned int start, unsigned int count, bool used)
{
    // Find bit to set it
    unsigned int start_byte = start / 8;
    unsigned int start_remainder = start % 8;

    unsigned int last_byte = (start + count) / 8;
    unsigned int last_remainder = (start + count) % 8;

    // Now to do some checking for cases
    if (start_byte == last_byte)
    {
        // Start and end is in the same byte
        // Example: Start: 0, count = 7, end = 7
        //	| 1 1 1 1 1 1 1 0 | 0 0 0 0 0 0	0 0 |
        //	| _____Byte0_____ | _____Byte1_____ |
        unsigned int bitsToSetAsBits = (1 << count) - 1; // Bit magic: 127

        if (used)
            gBitmap[start_byte] |= bitsToSetAsBits << (8 - start_remainder - count);
        else
            gBitmap[start_byte] &= ~(((char)bitsToSetAsBits << (8 - start_remainder - count)));
    }
    else if (last_byte == start_byte + 1 && start_remainder == 0 && last_remainder == 0)
    {
        // The block of slices fills exactly an entire byte
        gBitmap[start_byte] = used ? 0xFF : 0;
    }
    else
    {
        // Start and end is in different bytes
        unsigned int desired_start_pos = start_remainder;

        // 1. Set start
        // We know end is in a different byte, so we fill start byte, starting at desired pos
        unsigned int start_bits_to_set = 8 - desired_start_pos;
        unsigned int bitsToSetAsBits = ((1 << start_bits_to_set) - 1);
        if (used)
            gBitmap[start_byte] |= bitsToSetAsBits;
        else
            gBitmap[start_byte] &= ~bitsToSetAsBits;

        unsigned int bitsLeftToSet = count - start_bits_to_set;

        // 2. Set end (if not full)
        if (last_remainder > 0)
        {
            // We're only setting a specific amount of bits in the last byte
            unsigned int desired_end_pos = last_remainder;
            unsigned int bits_in_last_byte_to_set = last_remainder;
            bitsToSetAsBits = ((1 << bits_in_last_byte_to_set) - 1);

            if (used)
                gBitmap[last_byte] |= bitsToSetAsBits << (8 - desired_end_pos);
            else
                gBitmap[last_byte] &= ~(bitsToSetAsBits << (8 - desired_end_pos));

            bitsLeftToSet -= bits_in_last_byte_to_set;
        }

        // 3. Set all bytes inbetween
        if (bitsLeftToSet > 0)
        {
            unsigned int num_bytes_spanned = bitsLeftToSet / 8;
            unsigned int i;

            // start byte has already been set, and last byte has been set if it doesn't fill entire byte
            unsigned int last_byte_to_set = start_byte + 1 + num_bytes_spanned;

            for (i = start_byte + 1; i < last_byte_to_set; i++)
                gBitmap[i] = used ? 0xFF : 0;
        }
    }
}

static int get_first_available_slice(unsigned int requestedSize)
{
    unsigned int i, j;
    int clear_bits_start = -1;
    int clear_bits_found = 0;

    for (i = 0; i < MAX_ALLOCATED_SLICES / sizeof(char); i++)
    {
        if ((gBitmap[i] & 0xFF) == 0xFF) // No free in this byte, skip
        {
            clear_bits_start = -1;
            clear_bits_found = 0;
        }
        else if (gBitmap[i] == 0)
        {
            if (clear_bits_start == -1)
            {
                clear_bits_start = i * (sizeof(char)* 8); // Found a new start
                clear_bits_found = sizeof(char)* 8;
            }
            else
            {
                clear_bits_found += sizeof(char)* 8; // Add to the pile
            }
        }
        else
        {
            // OK - well that sucked, just loop and see how many clear bits we can find
            for (j = sizeof(char)* 8 - 1; j > 0; j--)
            {
                if ((gBitmap[i] & (1 << j)) == 0)
                {
                    // j contains the shifted offset from the right, set start to be the 0 based index from the left
                    if (clear_bits_start == -1)
                        clear_bits_start = ((i * (sizeof(char)* 8)) + (8 - j)) - 1;

                    clear_bits_found += 1;

                    if (clear_bits_found >= requestedSize)
                    {
                        break; // Found a free block
                    }
                }
                else
                {
                    clear_bits_start = -1;
                    clear_bits_found = 0;
                }
            }
        }

        // Did we find a free block?
        if (clear_bits_start != -1 && (unsigned int)clear_bits_found >= requestedSize)
        {
            break;
        }
    }

    // ~ assertions ~

    if (clear_bits_found < requestedSize)
    {
        uart_puts("Couldn't locate enough slices\n");
        return -1;
    }

    return clear_bits_start;
}

void* palloc(unsigned int size)
{
    int start_slice = -1;

    // Calculate number of slices necessary to store data
    unsigned int slice_count = (size / BYTES_PER_SLICE) + 1; // 1 slice for size

    if (size % BYTES_PER_SLICE)
        slice_count++;
    
    // Do we need an extended size byte?
    if (slice_count > 2147483647)// (Uint32 max value) - Invalid allocation size
        return 0; // TODO: Support larger allocations?

    // Now find slice_count clear consecutive bits in the bitmap
    start_slice = get_first_available_slice(slice_count);

    // Did we manage to locate a free block of slices?
    if (start_slice == -1)
        return 0; // BOO! Could not find a slice large enough

    // Calculate the address to the memory, and offset the size bytes
    unsigned char* ptr = gMemory + (start_slice * BYTES_PER_SLICE);

    // Write size bytes
    *ptr++ = ((slice_count >> 24) & 0xFF);
    *ptr++ = ((slice_count >> 16) & 0xFF);
    *ptr++ = ((slice_count >> 8) & 0xFF);
    *ptr++ = (slice_count & 0xFF);

    //Mark the slices as used in the bitmap
    mark_slices(start_slice, slice_count, true);

    gBytesAllocated += size + 1; // 1 for size bytes
    
    return ptr;
}

void phree(void* pointer)
{
    // Free da pointah
    unsigned char* ptr = (unsigned char*)pointer;
    int num_slices = -1;

    // Get slice count (Stored in 4 bytes preceeding the pointer)
    num_slices = (*(ptr - 4) << 24) |
        (*(ptr - 3) << 16) |
        (*(ptr - 2) << 8) |
        *(ptr - 1);

    ptr -= 4; // Make sure we deallocate the size bytes as well

    // ptr now points to the address, figure out the offset to find the slice start number
    unsigned int start_slice = (ptr - gMemory) / BYTES_PER_SLICE;

    mark_slices(start_slice, num_slices, false);

    gBytesAllocated -= num_slices * 4;
}