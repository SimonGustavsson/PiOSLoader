
#define KERNEL_MAX_SIZE 1048576 // (in Bytes, 1 MB)

// Shunt the dynamic memory after the kernel in memory
// Making sure it's not overwritten by the loading (slap it waaay behind!)
#define DYN_MEM_VA_START (0x8000 + KERNEL_MAX_SIZE * 2)

#define BYTES_PER_SLICE 4 
#define MAX_BYTES_PER_SIZE_BYTE ((2^7) - 1)
#define MAX_ALLOCATED_BYTES 104857600 // 100 MB, TODO: Don't hardcode this
#define MAX_ALLOCATED_SLICES (MAX_ALLOCATED_BYTES / BYTES_PER_SLICE) // 26214400
#define EXTENDED_SIZE_BYTE_FLAG (1 << 7)

// Initializes the allocator
void Pallocator_Initialize(void);

// Allocates 'size' bytes and returns a pointer to the newly allocated memory
void* palloc(unsigned int size);

// Allocates memory for an array of 'size' with elements of 'itemSize'
// Example: pcalloc(sizeof(int), 4); // Allocates an array of 4 ints
void* pcalloc(unsigned int itemSize, unsigned int size);

void phree(void* pointer);
void itoa(int number, char* buf);