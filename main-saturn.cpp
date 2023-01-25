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

template<typename T, int P>
inline constexpr T rgb(const vec3& color)
{
  constexpr int channel_mask = (1 << P) - 1;
  constexpr int last_bit = ((sizeof(T) * 8) - 1);

  vec3 c = functor1(clamp, color) * fp16_16(channel_mask);

  T red = static_cast<T>(c.r.value >> 16);
  T green = static_cast<T>(c.g.value >> 16);
  T blue = static_cast<T>(c.b.value >> 16);

  return (1 << last_bit)
       | (blue  << (P * 2))
       | (green << (P * 1))
       | (red   << (P * 0));
}

constexpr auto rgb15 = rgb<uint16_t, 5>;
constexpr auto rgb24 = rgb<uint32_t, 8>;

void put_pixel(int32_t x, int32_t y, const vec3& color)
{
  int sx = 320 / 2 + x;
  int sy = 240 / 2 - y;

  if (sx >= 320 || sx < 0 || sy >= 240 || sy < 0)
    return;

  vdp2.vram.u16[512 * sy + sx] = rgb15(color);
}

template <class T>
void fill(T * buf, T v, int32_t n) noexcept
{
  while (n > 0) {
    *buf++ = v;
    n -= (sizeof (T));
  }
}

void main_asdf()
{
  // DISP: Please make sure to change this bit from 0 to 1 during V blank.
  vdp2.reg.TVMD = ( TVMD__DISP | TVMD__LSMD__NON_INTERLACE
                  | TVMD__VRESO__240 | TVMD__HRESO__NORMAL_320);

  vdp2.reg.BGON = BGON__N0ON;

  vdp2.reg.CHCTLA = (
                      CHCTLA__N0CHCN__32K_COLOR // 15 bits per pixel, RGB
                      //CHCTLA__N0CHCN__16M_COLOR // 24 bits per pixel
                    | CHCTLA__N0BMSZ__512x256_DOT
                    | CHCTLA__N0BMEN__BITMAP_FORMAT
                    );

  vdp2.reg.MPOFN = MPOFN__N0MP(0);

  constexpr s32 plane_size = 512 * 256 * 2;
  fill<volatile uint32_t>(&vdp2.vram.u32[0x0 / 4], (1 << 31) | (1 << 15), plane_size);

  vdp2.reg.SCXIN0 = 0;
  vdp2.reg.SCXDN0 = 0;
  vdp2.reg.SCYIN0 = 0;
  vdp2.reg.SCYDN0 = 0;
  vdp2.reg.ZMXIN0 = 1;
  vdp2.reg.ZMXDN0 = 0;
  vdp2.reg.ZMYIN0 = 1;
  vdp2.reg.ZMYDN0 = 0;

  vdp2.reg.VCSTA = 0;

  vdp2.reg.WCTLA = 0;
  vdp2.reg.WCTLB = 0;
  vdp2.reg.WCTLC = 0;

  render(put_pixel);
}

extern "C"
void start(void)
{
  main_asdf();
  while (1) {}
}
