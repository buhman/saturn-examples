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
#include "../common/vdp2_func.h"

extern void * _butterfly_data_pal_start __asm("_binary_res_butterfly_data_pal_start");
extern void * _butterfly_data_pal_size __asm("_binary_res_butterfly_data_pal_size");

extern void * _butterfly_data_start __asm("_binary_res_butterfly_data_start");
extern void * _butterfly_data_size __asm("_binary_res_butterfly_data_size");

inline constexpr uint16_t rgb15(const uint8_t * buf)
{
  return ((buf[2] >> 3) << 10) // blue
       | ((buf[1] >> 3) << 5)  // green
       | ((buf[0] >> 3) << 0); // red
}

void palette_data()
{
  const uint32_t buf_size = reinterpret_cast<uint32_t>(&_butterfly_data_pal_size);
  const uint8_t * buf = reinterpret_cast<uint8_t*>(&_butterfly_data_pal_start);
  uint32_t buf_ix = 0;

  for (uint32_t i = 0; i < (buf_size / 3); i++) {
    vdp2.cram.u16[i] = rgb15(&buf[buf_ix]);
    buf_ix += 3;
  }
}

void cell_data()
{
  constexpr int page_width = 64;
  constexpr int pixel_width = 64;

  const uint8_t * buf = reinterpret_cast<uint8_t*>(&_butterfly_data_start);

  for (int j = 0; j < 8; j++) {
    for (int i = 0; i < 8; i++) {
      for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
          vdp2.vram.u16[(page_width * ((j * 8) + i)) + (y * 8 + x)] =
            buf[(y + (j * 8)) * pixel_width + (x + (8 * i))];
        }
      }
    }
  }
}

template <typename T>
void fill(T * buf, T v, int32_t n) noexcept
{
  while (n > 0) {
    *buf++ = v;
    n -= (sizeof (T));
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
  vdp2.reg.BGON = BGON__N0ON;

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
  vdp2.reg.MPABN0 = MPABN0__N0MPB(0) | MPABN0__N0MPA(plane_a); // bits 5~0
  vdp2.reg.MPCDN0 = MPABN0__N0MPD(0) | MPABN0__N0MPC(0); // bits 5~0

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
  for (int y = 0; y < cell_vertical; y++) {
    for (int x = 0; x < cell_horizontal; x++) {
      vdp2.vram.u16[(plane_a_offset / 2) + (y * page_width) + x]
        = character_offset * ((y * cell_horizontal) + x);
    }
  }

  palette_data();
  cell_data();
}
