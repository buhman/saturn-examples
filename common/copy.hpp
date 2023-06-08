#pragma once

#include <stdint.h>

template <typename T>
inline void copy(T * dst, const T * src, int32_t n) noexcept
{
  n = n / (sizeof (T));
  while (n > 0) {
    *dst++ = *src++;
    n--;
  }
}

template <typename T>
inline void fill(T * dst, const T src, int32_t n) noexcept
{
  n = n / (sizeof (T));
  while (n > 0) {
    *dst++ = src;
    n--;
  }
}

template <typename T>
inline void move(T * dst, const T * src, int32_t n) noexcept
{
  n = n / (sizeof (T));
  if (dst < src) {
    //   d   s
    // 0123456789
    while (n > 0) {
      *dst++ = *src++;
      n--;
    }
  } else {
    //   s   d
    // 0123456789
    while (n) {
      n--;
      dst[n] = src[n];
    }
  }
}
