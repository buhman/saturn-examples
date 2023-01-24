#include <iostream>
#include <cstdint>

using namespace std;

#include "vec.hpp"
#include "fp.hpp"
#include "raytracing.hpp"

typedef vec<3, uint8_t> pixel;
static pixel frame[canvas::height][canvas::width] = { 0 };

fp16_16 clamp(fp16_16 const& n)
{
  return (n > fp16_16(1) ? fp16_16(1) : (n < fp16_16(0) ? fp16_16(0) : n));
};

uint8_t to_uint8_t(fp16_16 const& v)
{
  return static_cast<uint8_t>(v.value >> 16);
}

void put_pixel(int32_t x, int32_t y, const vec3& color)
{
  using namespace canvas;

  int sx = width / 2 + x;
  int sy = height / 2 - y;

  if (!(sx >= 0 && sx < width && sy >= 0 && sy < height)) {
    return;
  }

  vec3 px255 = functor1(clamp, color) * fp16_16(255);
  frame[sy][sx] = functor1(to_uint8_t, px255);
}

void render_ppm(ostream& out)
{
  using namespace canvas;

  out << "P3 " << width << ' ' << height << " 255\n";
  for (int sy = 0; sy < height; sy++) {
    for (int sx = 0; sx < width; sx++) {
      const pixel& px = frame[sy][sx];
      out << +px.r << ' ' << +px.g << ' ' << +px.b << '\n';
    }
  }
}

int main()
{
  render(put_pixel);
}
