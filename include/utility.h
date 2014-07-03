#ifndef UTILITY_H
#define UTILITY_H

static inline int my_strlen(char* str)
{
	int length = 0;
	while(*str++)
		length++;

	return length;
}

static inline char* my_strcpy_s(const char* src, unsigned int dest_len, char* dst)
{
    char* ptr;
    ptr = dst;
    unsigned int num_written = 0;
    while ((*dst++ = *src++) && (num_written++ < dest_len));

    return ptr;
}

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

#endif