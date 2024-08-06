// A crude implementation of string operations used by this DB example system.

#include <stdint.h>
#include <limits.h>

#define ALIGN (sizeof(size_t))
#define ONES ((size_t)-1/UCHAR_MAX)
#define HIGHS (ONES * (UCHAR_MAX/2+1))
#define HASZERO(x) ((x)-ONES & ~(x) & HIGHS)
#define SS (sizeof(size_t))

/* Very simple memzero implementation */
static void memzero(void *dst, size_t n) {
    for (int i = 0; i < n; i++) {
        ((char *)dst)[i] = 0;
    }
}

void *memcpy(void *__restrict dest, const void *__restrict src, size_t size) {
    char *destination = (char *) dest;
    char *source = (char *) src;

    for (size_t i = 0; i < size; i++) {
        destination[i] = source[i];
    }
    return destination;
}

void *memchr(const void *src, int c, size_t n)
{
	const unsigned char *s = src;
	c = (unsigned char)c;
	for (; ((uintptr_t)s & ALIGN) && n && *s != c; s++, n--);
	if (n && *s != c) {
		const size_t *w;
		size_t k = ONES * c;
		for (w = (const void *)s; n>=SS && !HASZERO(*w^k); w++, n-=SS);
		for (s = (const void *)w; n && *s != c; s++, n--);
	}
	return n ? (void *)s : 0;
}

size_t strnlen(const char *s, size_t n) {
	const char *p = memchr(s, 0, n);
	return p ? p-s : n;
}

size_t strlen(const char *s)
{
	const char *a = s;
	const size_t *w;
	for (; (uintptr_t)s % ALIGN; s++) if (!*s) return s-a;
	for (w = (const void *)s; !HASZERO(*w); w++);
	for (s = (const void *)w; *s; s++);
	return s-a;
}

char *strncpy(char *__restrict dest, const char *__restrict src, size_t max_len) {
    size_t i = 0;
	const unsigned char *s = src;
	unsigned char *d = dest;
	while ((*d++ = *s++) && i < max_len);
	return dest;
}

char *strcpy(char *restrict dest, const char *restrict src) {
	const unsigned char *s = src;
	unsigned char *d = dest;
	while ((*d++ = *s++));
	return dest;
}

char *strcat(char *restrict dest, const char *restrict src)
{
	strcpy(dest + strlen(dest), src);
	return dest;
}

