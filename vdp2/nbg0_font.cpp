#include <stdint.h>

#include "vdp2.h"
#include "../common/vdp2_func.hpp"

#include "font/hp_100lx_4bit.data.h"

void cell_data()
{
  const uint32_t * start = reinterpret_cast<uint32_t *>(&_binary_font_hp_100lx_4bit_data_start);
  const int size = reinterpret_cast<uint32_t>(&_binary_font_hp_100lx_4bit_data_size);

  for (int i = 0; i < (size / 4); i++) {
    vdp2.vram.u32[i] = start[i];
  }
}

void palette_data()
{
  vdp2.cram.u16[0] = 0x0000;
  vdp2.cram.u16[1] = 0xffff;
}

void main()
{
  v_blank_in();

  // DISP: Please make sure to change this bit from 0 to 1 during V blank.
  vdp2.reg.TVMD = ( TVMD__DISP | TVMD__LSMD__NON_INTERLACE
                  | TVMD__VRESO__240 | TVMD__HRESO__NORMAL_320);

  /* set the color mode to 5bits per channel, 1024 colors */
  vdp2.reg.RAMCTL = RAMCTL__CRMD__RGB_5BIT_1024
                  | RAMCTL__VRAMD | RAMCTL__VRBMD;

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
  constexpr int plane_a = 1;
  constexpr int plane_a_offset = plane_a * 0x4000;

  constexpr int page_size = 64 * 64 * 2; // N0PNB__1WORD (16-bit)
  constexpr int plane_size = page_size * 1;

  vdp2.reg.CYCA0 = 0x0F44F99F;
  vdp2.reg.CYCA1 = 0x0F44F99F;
  vdp2.reg.CYCB0 = 0x0F44F99F;
  vdp2.reg.CYCB1 = 0x0F44F99F;

  vdp2.reg.MPOFN = MPOFN__N0MP(0); // bits 8~6
  vdp2.reg.MPABN0 = MPABN0__N0MPB(plane_a) | MPABN0__N0MPA(plane_a); // bits 5~0
  vdp2.reg.MPCDN0 = MPCDN0__N0MPD(plane_a) | MPCDN0__N0MPC(plane_a); // bits 5~0

  palette_data();
  cell_data();

  vdp2.reg.PNCN0 = PNCN0__N0PNB__2WORD;
  for (int i = 0; i < 64 * 64; i++) {
    vdp2.vram.u32[(plane_a_offset / 4) + i] = ' ' - 0x20;
  }

  const char * test = "conversion from 8";
  int ix = 0;
  while (*test) {
    uint8_t c = *test++;
    vdp2.vram.u32[(plane_a_offset / 4) + ix++] = c - 0x20;
  }

  while (1);
}
