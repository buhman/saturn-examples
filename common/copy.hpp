#include <stdint.h>

template <typename T>
inline void copy(T * dst, const T * src, int32_t n) noexcept
{
  while (n > 0) {
    *dst++ = *src++;
    n -= (sizeof (T));
  }
}

template <typename T>
inline void fill(T * dst, const T src, int32_t n) noexcept
{
  while (n > 0) {
    *dst++ = src;
    n -= (sizeof (T));
  }
}
