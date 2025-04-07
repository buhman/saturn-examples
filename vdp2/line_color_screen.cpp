/*
  color data

  255 colors, 3 bytes per channel, packed
*/
/*
  image data

  64*64 px, 1 byte per palette index
*/

#include <stdint.h>

#include "vdp2.h"
#include "../common/vdp2_func.hpp"

inline constexpr uint16_t rgb555(int r, int g, int b)
{
  if (r > 255) r = 255;
  if (g > 255) g = 255;
  if (b > 255) b = 255;
  if (r < 0) r = 0;
  if (g < 0) g = 0;
  if (b < 0) b = 0;

  return ((b >> 3) << 10) // blue
       | ((g >> 3) << 5)  // green
       | ((r >> 3) << 0); // red
}

template <typename T>
void fill(T * buf, T v, int32_t n) noexcept
{
  while (n > 0) {
    *buf++ = v;
    n -= (sizeof (T));
  }
}

struct color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct interp {
  int line;
  struct color color;
};

struct interp gradient[] = {
  {  0, {0x59, 0x6b, 0x88}},
  { 40, {0x59, 0x6b, 0x88}},
  { 80, {0xa8, 0x34, 0x14}},
  {110, {0xac, 0xa5, 0x30}},
  {120, {0xac, 0xa5, 0x30}},
  {140, {0xa8, 0x34, 0x14}},
  {999, {0xa8, 0x34, 0x14}},
};

const int gradient_len = (sizeof (gradient)) / (sizeof (gradient[0]));

struct color lerp(struct color a, struct color b, int start, int end, int y)
{
  if (start == end) {
    return (struct color){a.r, a.g, a.b};
  }
  if (end == 999) {
    return (struct color){a.r, a.g, a.b};
  }

  int dr = ((int)b.r - (int)a.r);
  int dg = ((int)b.g - (int)a.g);
  int db = ((int)b.b - (int)a.b);

  int t1 = end - start;
  int t0 = y - start;

  return (struct color){
    (uint8_t)((int)a.r + dr * t0 / t1),
    (uint8_t)((int)a.g + dg * t0 / t1),
    (uint8_t)((int)a.b + db * t0 / t1),
  };
}

void gen_gradient()
{
  struct interp * prev = &gradient[0];
  struct interp * cur = prev;

  int i = 0;

  for (int y = 0; y < 240; y++) {
    if (y >= cur->line) {
      prev = cur;
      i += 1;
      cur = &gradient[i];
    }

    struct color c = lerp(prev->color, cur->color, prev->line, cur->line, y);
    vdp2.vram.u16[y] = rgb555(c.r, c.g, c.b);
    //vdp2.vram.u16[y] = rgb555(cur->color.r, cur->color.g, cur->color.b);
  }
}

void main()
{
  v_blank_in();

  // DISP: Please make sure to change this bit from 0 to 1 during V blank.
  vdp2.reg.TVMD = ( TVMD__DISP | TVMD__LSMD__NON_INTERLACE
                  | TVMD__VRESO__240 | TVMD__HRESO__NORMAL_320);


  /* set the color mode to 5bits per channel, 1024 colors */
  vdp2.reg.RAMCTL = RAMCTL__CRMD__RGB_5BIT_1024;

  /* enable display of NBG0 */
  //vdp2.reg.BGON = BGON__N0ON;

  /* set character format for NBG0 to palettized 2048 color
     set enable "cell format" for NBG0
     set character size for NBG0 to 1x1 cell */
  vdp2.reg.CHCTLA = CHCTLA__N0CHCN__2048_COLOR
                  | CHCTLA__N0BMEN__CELL_FORMAT
                  | CHCTLA__N0CHSZ__1x1_CELL;
  /* "Note: In color RAM modes 0 and 2, 2048-color becomes 1024-color" */

  /* use 1-word (16-bit) pattern names */
  vdp2.reg.PNCN0 = PNCN0__N0PNB__1WORD;

  /* plane size */
  vdp2.reg.PLSZ = PLSZ__N0PLSZ__1x1;

  /* map plane offset
     1-word: value of bit 6-0 * 0x2000
     2-word: value of bit 5-0 * 0x4000
  */
  constexpr int plane_a = 2;
  constexpr int plane_a_offset = plane_a * 0x2000;

  constexpr int page_size = 64 * 64 * 2; // N0PNB__1WORD (16-bit)
  constexpr int plane_size = page_size * 1;

  vdp2.reg.MPOFN = MPOFN__N0MP(0); // bits 8~6
  vdp2.reg.MPABN0 = MPABN0__N0MPB(plane_a) | MPABN0__N0MPA(plane_a); // bits 5~0
  vdp2.reg.MPCDN0 = MPCDN0__N0MPD(plane_a) | MPCDN0__N0MPC(plane_a); // bits 5~0

  constexpr int cell_size = (8 * 8) * 2; // N0CHCN__2048_COLOR (16-bit)
  constexpr int character_size = cell_size * (1 * 1); // N0CHSZ__1x1_CELL
  constexpr int character_offset = character_size / 0x20;

  // zeroize character/cell data from 0 up to plane_a_offset
  fill<uint32_t>(&vdp2.vram.u32[(0 / 2)], 0, plane_a_offset);

  // "zeroize" plane_a to the 0x40th (64th) character index (an unused/transparent character)
  // this creates the '40' indexes in the below picture
  fill<uint16_t>(&vdp2.vram.u16[(plane_a_offset / 2)], character_offset * 0x40, plane_size);

  constexpr int pixel_width = 64;
  constexpr int pixel_height = 64;
  constexpr int cell_horizontal = pixel_width / 8;
  constexpr int cell_vertical = pixel_height / 8;

  constexpr int page_width = 64;

  /*
  set plane_a character index data to this (hex):

                            |
     00 01 02 03 04 05 06 07 40 40 ..
     08 09 0a 0b 0c 0d 0e 0f 40 40 ..
     10 11 12 13 14 15 16 17 40 40 ..
     18 19 1a 1b 1c 1d 1e 1f 40 40 ..
     20 21 22 23 24 25 26 27 40 40 ..
     28 29 2a 2b 2c 2d 2e 2f 40 40 ..
     30 31 32 33 34 35 36 37 40 40 ..
  -- 38 39 3a 3b 3c 3d 3e 3f 40 40 ..
     40 40 40 40 40 40 40 40 40 40 ..
     40 40 40 40 40 40 40 40 40 40 ..
     .. .. .. .. .. .. .. .. .. .. ..

  (the above numbers are not pre-multiplied by character_offset)
  */

  vdp2.reg.BKTA = BKTA__BKCLMD_PER_LINE;

  gen_gradient();
}
