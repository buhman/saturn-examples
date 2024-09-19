#include <stdint.h>

void *memcpy(void *restrict dest, const void *restrict src, uint32_t n)
{
  unsigned char *d = dest;
  const unsigned char *s = src;

  for (; n; n--) *d++ = *s++;
  return dest;
}
