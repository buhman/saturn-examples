#pragma once

#include "../math/fp.hpp"
#include "raytracing.hpp"

using vec3 = vec<3, fp16_16>;

namespace viewport {
  constexpr int width = 1;
  constexpr int height = 1;
}

namespace canvas {
  constexpr int square_width = 256;
  constexpr int square_height = 256;
  constexpr int width = 320;
  constexpr int height = 240;
}

void render(int half, void (&put_pixel) (int32_t x, int32_t y, const vec3& c));
