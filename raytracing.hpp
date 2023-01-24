#pragma once

#include "fp.hpp"
#include "raytracing.hpp"

using vec3 = vec<3, fp16_16>;

namespace canvas {
  constexpr int bit_width = 8;
  constexpr int bit_height = 8;
  constexpr int width = (1 << bit_width);
  constexpr int height = (1 << bit_height);
}

void render(void (&put_pixel) (int32_t x, int32_t y, const vec3& c));
