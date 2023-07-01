#pragma once

#include <cstdint>
#include <concepts>

#include "div.hpp"

struct fp_raw_tag {};

template <typename T, int B>
constexpr inline int fp_fractional(std::floating_point auto n)
{
  return (n - static_cast<T>(n)) * (1 << B);
}

template <typename T, int B>
constexpr inline int fp_integral(std::floating_point auto n)
{
  return static_cast<T>(n) << B;
}

template <typename T, typename I, int B>
struct fp
{
  T value;

  constexpr inline fp(std::integral auto n)
    : value(n * (1 << B))
  {}

  consteval inline fp(std::floating_point auto n)
    : value(fp_integral<T, B>(n) + fp_fractional<T, B>(n))
  {}

  constexpr inline explicit fp(T n, struct fp_raw_tag)
    : value(n)
  {}

  constexpr inline explicit operator int() const
  {
    return value >> 16;
  }

  constexpr inline fp<T, I, B> operator-()
  {
    return fp(-value, fp_raw_tag{});
  }

  inline constexpr fp<T, I, B>& operator=(fp<T, I, B> const& v);

  inline constexpr fp<T, I, B>& operator+=(fp<T, I, B> const& v);

  inline constexpr fp<T, I, B>& operator-=(fp<T, I, B> const& v);

  inline constexpr fp<T, I, B>& operator*=(fp<T, I, B> const& v);
};

template <typename T, typename I, int B>
inline constexpr fp<T, I, B>& fp<T, I, B>::operator=(fp<T, I, B> const& v)
{
  this->value = v.value;
  return *this;
}

template <typename T, typename I, int B>
inline constexpr fp<T, I, B>& fp<T, I, B>::operator+=(fp<T, I, B> const& v)
{
  *this = *this + v;
  return *this;
}

template <typename T, typename I, int B>
inline constexpr fp<T, I, B>& fp<T, I, B>::operator-=(fp<T, I, B> const& v)
{
  *this = *this - v;
  return *this;
}

template <typename T, typename I, int B>
inline constexpr fp<T, I, B>& fp<T, I, B>::operator*=(fp<T, I, B> const& v)
{
  *this = *this * v;
  return *this;
}

template <typename T, typename I, int B>
constexpr inline fp<T, I, B> operator+(const fp<T, I, B>& a, const fp<T, I, B>& b)
{
  return fp<T, I, B>(a.value + b.value, fp_raw_tag{});
}

template <typename T, typename I, int B>
constexpr inline fp<T, I, B> operator-(const fp<T, I, B>& a, const fp<T, I, B>& b)
{
  return fp<T, I, B>(a.value - b.value, fp_raw_tag{});
}

template <typename T, typename I, int B>
constexpr inline fp<T, I, B> operator*(const fp<T, I, B>& a, const fp<T, I, B>& b)
{
  const I p = (static_cast<I>(a.value) * static_cast<I>(b.value));
  return fp<T, I, B>(static_cast<T>(p >> B), fp_raw_tag{});
}

template <typename T, typename I, int B>
constexpr inline fp<T, I, B> operator*(const fp<T, I, B>& a, T b)
{
  I p = (static_cast<I>(a.value) * static_cast<I>(b));
  return fp<T, I, B>(static_cast<T>(p), fp_raw_tag{});
}

template <typename T, typename I, int B>
constexpr inline fp<T, I, B> operator*(T b, const fp<T, I, B>& a)
{
  I p = (static_cast<I>(a.value) * static_cast<I>(b));
  return fp<T, I, B>(static_cast<T>(p), fp_raw_tag{});
}

template <typename T, typename I, int B>
constexpr inline fp<T, I, B> operator/(const fp<T, I, B>& a, const fp<T, I, B>& b)
{
  //T p = (static_cast<T>(a.value) * ) / static_cast<T>(b.value);
  //T p = static_cast<T>(a.value) / static_cast<T>(b.value);
  I p = __div64_32((static_cast<I>(a.value) << 16), static_cast<T>(b.value));

  return fp<T, I, B>(static_cast<T>(p), fp_raw_tag{});
}

// comparison

template <typename T, typename I, int B>
constexpr inline bool operator==(const fp<T, I, B>& a, const fp<T, I, B>& b)
{
    return a.value == b.value;
}

template <typename T, typename I, int B>
constexpr inline bool operator!=(const fp<T, I, B>& a, const fp<T, I, B>& b)
{
    return a.value != b.value;
}

template <typename T, typename I, int B>
constexpr inline bool operator<(const fp<T, I, B>& a, const fp<T, I, B>& b)
{
    return a.value < b.value;
}

template <typename T, typename I, int B>
constexpr inline bool operator>(const fp<T, I, B>& a, const fp<T, I, B>& b)
{
    return a.value > b.value;
}

template <typename T, typename I, int B>
constexpr inline bool operator<=(const fp<T, I, B>& a, const fp<T, I, B>& b)
{
    return a.value <= b.value;
}

template <typename T, typename I, int B>
constexpr inline bool operator>=(const fp<T, I, B>& a, const fp<T, I, B>& b)
{
    return a.value >= b.value;
}

// limits

template <typename F>
struct fp_limits;

template <typename T, typename I, int B>
struct fp_limits<fp<T, I, B>>
{
  static constexpr fp<T, I, B> min()
  {
    return fp<T, I, B>(-(1 << (2 * B - 1)), fp_raw_tag{});
  }

  static constexpr fp<T, I, B> max()
  {
    return fp<T, I, B>((static_cast<I>(1) << (2 * B - 1)) - 1, fp_raw_tag{});
  }
};

// functions

template <typename T, typename I, int B>
constexpr inline fp<T, I, B> pow(fp<T, I, B> a, fp<T, I, B> b)
{
  while (b > fp<T, I, B>(1)) {
    a *= a;
    b -= fp<T, I, B>(1);
  }
  return a;
}

// specializations

using fp16_16 = fp<int32_t, int64_t, 16>;

constexpr inline fp16_16 sqrt(const fp16_16& n)
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
