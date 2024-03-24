#include <stdint.h>

#include "vdp2.h"
#include "../common/vdp2_func.hpp"


extern void * _kirby_data_pal_start __asm("_binary_res_kirby_data_pal_start");
extern void * _kirby_data_pal_size __asm("_binary_res_kirby_data_pal_size");

extern void * _kirby_data_start __asm("_binary_res_kirby_data_start");
extern void * _kirby_data_size __asm("_binary_res_kirby_data_size");

inline constexpr uint16_t rgb15(const uint8_t * buf)
{
  return ((buf[2] >> 3) << 10) // blue
       | ((buf[1] >> 3) << 5)  // green
       | ((buf[0] >> 3) << 0); // red
}

void palette_data()
{
  const uint32_t buf_size = reinterpret_cast<uint32_t>(&_kirby_data_pal_size);
  const uint8_t * buf = reinterpret_cast<uint8_t*>(&_kirby_data_pal_start);
  uint32_t buf_ix = 0;

  for (uint32_t i = 0; i < (buf_size / 3); i++) {
    vdp2.cram.u16[i] = rgb15(&buf[buf_ix]);
    buf_ix += 3;
  }
}

uint32_t cell_data(const uint32_t top)
{
  const uint32_t buf_size = reinterpret_cast<uint32_t>(&_kirby_data_size);
  const uint8_t * buf = reinterpret_cast<uint8_t*>(&_kirby_data_start);

  // round to nearest multiple of 32
  const uint32_t table_size = ((buf_size / 2) + 0x20 - 1) & (-0x20);
  const uint32_t base_address = top - table_size; // in bytes

  // px is a conversion from 8 bits per index to 4 bits per index
  // the value is shifted to its position in a 32-bit big-endian value
#define px(x) (buf[i * 8 + ((x) & 0xf)] << ((7 - ((x) & 0xf)) * 4))

  for (uint32_t i = 0; i < (buf_size / 8); i++) {
    // write an entire row all at once
    vdp2.vram.u32[(base_address / 4) + i] =
      //vdp2.vram.u32[i] =
      px(0) | px(1) | px(2) | px(3) | px(4) | px(5) | px(6) | px(7);
  }

#undef px

  return base_address;
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

  vdp2.reg.VRSIZE = 0;

  /* enable display of NBG0 */
  vdp2.reg.BGON = BGON__N0ON | BGON__N0TPON;

  /* set character format for NBG0 to palettized 16 color
     set enable "cell format" for NBG0
     set character size for NBG0 to 1x1 cell */
  vdp2.reg.CHCTLA = CHCTLA__N0CHCN__16_COLOR
                  | CHCTLA__N0BMEN__CELL_FORMAT
                  | CHCTLA__N0CHSZ__1x1_CELL;

  /* plane size */
  vdp2.reg.PLSZ = PLSZ__N0PLSZ__1x1;

  /* map plane offset
     1-word: value of bit 6-0 * 0x2000
     2-word: value of bit 5-0 * 0x4000
  */
  constexpr int plane_a = 0;
  constexpr int plane_a_offset = plane_a * 0x4000;

  constexpr int page_size = 64 * 64 * 2; // N0PNB__1WORD (16-bit)
  constexpr int plane_size = page_size * 1;

  vdp2.reg.CYCA0 = 0x0F44F99F;
  vdp2.reg.CYCB0 = 0x0F44F99F;

  vdp2.reg.MPOFN = MPOFN__N0MP(0); // bits 8~6
  vdp2.reg.MPABN0 = MPABN0__N0MPB(0) | MPABN0__N0MPA(plane_a); // bits 5~0
  vdp2.reg.MPCDN0 = MPCDN0__N0MPD(0) | MPCDN0__N0MPC(0); // bits 5~0

  uint32_t top = (sizeof (union vdp2_vram));
  palette_data();
  uint32_t kirby_address = top = cell_data(top);
  uint32_t pattern_name = kirby_address / 32;

  /* use 1-word (16-bit) pattern names */
  //vdp2.reg.PNCN0 = PNCN0__N0PNB__1WORD | PNCN0__N0CNSM | PNCN0__N0SCN((pattern_name >> 10) & 0x1f);
  //fill<uint16_t>(&vdp2.vram.u16[(plane_a_offset / 4)], pattern_name & 0xfff, plane_size);

  /* use 2-word (32-bit) pattern names */
  vdp2.reg.PNCN0 = PNCN0__N0PNB__2WORD;
  fill<uint32_t>(&vdp2.vram.u32[(plane_a_offset / 4)], pattern_name, plane_size);

  // both 1-word and 2-word have identical behavior; 2-word is enabled to reduce/focus suspicion.
}
