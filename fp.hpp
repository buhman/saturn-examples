#pragma once

#include <stdint.h>

struct fp_raw_tag {};

template <typename T, typename I, int B>
struct fp
{
  T value;

  constexpr inline fp(T n) noexcept
    : value(n * (1 << B))
  {}

  constexpr inline explicit fp(T n, struct fp_raw_tag) noexcept
    : value(n)
  {}

  constexpr inline fp<T, I, B> operator-() const noexcept
  {
    return fp(-value, fp_raw_tag{});
  }
};

template <typename T, typename I, int B>
constexpr inline fp<T, I, B> operator+(const fp<T, I, B>& a, const fp<T, I, B>& b) noexcept
{
  return fp<T, I, B>(a.value + b.value, fp_raw_tag{});
}

template <typename T, typename I, int B>
constexpr inline fp<T, I, B> operator-(const fp<T, I, B>& a, const fp<T, I, B>& b) noexcept
{
  return fp<T, I, B>(a.value - b.value, fp_raw_tag{});
}

template <typename T, typename I, int B>
constexpr inline fp<T, I, B> operator*(const fp<T, I, B>& a, const fp<T, I, B>& b) noexcept
{
  I p = (static_cast<I>(a.value) * static_cast<I>(b.value));
  return fp<T, I, B>(static_cast<T>(p >> B), fp_raw_tag{});
}

template <typename T, typename I, int B>
constexpr inline fp<T, I, B> operator*(const fp<T, I, B>& a, T b) noexcept
{
  I p = (static_cast<I>(a.value) * static_cast<I>(b));
  return fp<T, I, B>(static_cast<T>(p), fp_raw_tag{});
}

template <typename T, typename I, int B>
constexpr inline fp<T, I, B> operator*(T b, const fp<T, I, B>& a) noexcept
{
  I p = (static_cast<I>(a.value) * static_cast<I>(b));
  return fp<T, I, B>(static_cast<T>(p), fp_raw_tag{});
}

template <typename T, typename I, int B>
constexpr inline fp<T, I, B> operator/(const fp<T, I, B>& a, const fp<T, I, B>& b) noexcept
{
  I p = (static_cast<I>(a.value) * (static_cast<I>(1) << B)) / static_cast<I>(b.value);
  return fp<T, I, B>(static_cast<T>(p), fp_raw_tag{});
}

// comparison

template <typename T, typename I, int B>
constexpr inline bool operator==(const fp<T, I, B>& a, const fp<T, I, B>& b) noexcept
{
    return a.value == b.value;
}

template <typename T, typename I, int B>
constexpr inline bool operator!=(const fp<T, I, B>& a, const fp<T, I, B>& b) noexcept
{
    return a.value != b.value;
}

template <typename T, typename I, int B>
constexpr inline bool operator<(const fp<T, I, B>& a, const fp<T, I, B>& b) noexcept
{
    return a.value < b.value;
}

template <typename T, typename I, int B>
constexpr inline bool operator>(const fp<T, I, B>& a, const fp<T, I, B>& b) noexcept
{
    return a.value > b.value;
}

template <typename T, typename I, int B>
constexpr inline bool operator<=(const fp<T, I, B>& a, const fp<T, I, B>& b) noexcept
{
    return a.value <= b.value;
}

template <typename T, typename I, int B>
constexpr inline bool operator>=(const fp<T, I, B>& a, const fp<T, I, B>& b) noexcept
{
    return a.value >= b.value;
}

// limits

template <typename F>
struct fp_limits;

template <typename T, typename I, int B>
struct fp_limits<fp<T, I, B>>
{
  static constexpr fp<T, I, B> min() noexcept
  {
    return fp<T, I, B>(-(1 << (2 * B - 1)), fp_raw_tag{});
  }

  static constexpr fp<T, I, B> max() noexcept
  {
    return fp<T, I, B>((static_cast<I>(1) << (2 * B - 1)) - 1, fp_raw_tag{});
  }
};

// specializations

using fp16_16 = fp<int32_t, int64_t, 16>;

constexpr fp16_16 sqrt(fp16_16 n) noexcept
{
  int32_t x = n.value;
  int32_t c = 0;
  int32_t d = 1 << 30;
  while (d > (1 << 6))
  {
    int32_t t = c + d;
    if (x >= t)
    {
      x -= t;
      c = t + d;
    }
    x <<= 1;
    d >>= 1;
  }
  return fp16_16(c >> 8, fp_raw_tag{});
}
