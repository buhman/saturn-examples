#pragma once

template <typename T>
static inline constexpr T min(T a, T b)
{
  return a > b ? b : a;
}

template <typename T>
static inline constexpr T max(T a, T b)
{
  return a > b ? a : b;
}
