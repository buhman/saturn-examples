#include "vdp2.h"
#include "vdp1.h"
#include "scu.h"
#include "smpc.h"
#include "sh2.h"

#include "vec.hpp"
#include "fp.hpp"
#include "raytracing.hpp"

fp16_16 clamp(fp16_16 const& n)
{
  return (n > fp16_16(1) ? fp16_16(1) : (n < fp16_16(0) ? fp16_16(0) : n));
};

uint16_t rgb15(const vec3& color)
{
  vec3 c = functor1(clamp, color) * fp16_16(255);

  uint8_t red = (c.r.value >> 16) & 0xff;
  uint8_t green = (c.g.value >> 16) & 0xff;
  uint8_t blue = (c.b.value >> 16) & 0xff;

  return (blue << 10) | (green << 5) | (red << 0);
}

void put_pixel(int32_t x, int32_t y, const vec3& color)
{
  int sx = 320 / 2 + x;
  int sy = 240 / 2 - y;

  vdp2.vram.u16[512 * sy + sx] = (1 << 15) | rgb15(color);
}

void main_asdf()
{
  // DISP: Please make sure to change this bit from 0 to 1 during V blank.
  vdp2.reg.TVMD = ( TVMD__DISP | TVMD__LSMD__NON_INTERLACE
                  | TVMD__VRESO__240 | TVMD__HRESO__NORMAL_320);

  vdp2.reg.BGON = BGON__N0ON;

  vdp2.reg.CHCTLA = ( CHCTLA__N0CHCN__32K_COLOR     // 15 bits per pixel, RGB
                    | CHCTLA__N0BMSZ__512x256_DOT
                    | CHCTLA__N0BMEN__BITMAP_FORMAT
                    );

  vdp2.reg.MPOFN = MPOFN__N0MP(0);

  render(put_pixel);
}

extern "C"
void start(void)
{
  main_asdf();
  while (1) {}
}
