#include <stdint.h>

#include "vdp2.h"
#include "../common/vdp2_func.hpp"

#include "font/hp_100lx_4bit.data.h"

void cell_data()
{
  const uint32_t * start = reinterpret_cast<uint32_t *>(&_binary_font_hp_100lx_4bit_data_start);
  const int size = reinterpret_cast<uint32_t>(&_binary_font_hp_100lx_4bit_data_size);

  // the start of VRAM-A0
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
                  | RAMCTL__VRAMD
                  | RAMCTL__VRBMD
                  | RAMCTL__RDBSA0__CHARACTER_PATTERN_TABLE // VRAM-A0 0x000000
                  | RAMCTL__RDBSA1__PATTERN_NAME_TABLE;     // VRAM-A1 0x020000

  vdp2.reg.VRSIZE = 0;

  /* enable display of NBG0 */
  vdp2.reg.BGON = BGON__R0ON | BGON__R0TPON;

  /* set character format for NBG0 to palettized 16 color
     set enable "cell format" for NBG0
     set character size for NBG0 to 1x1 cell */
  vdp2.reg.CHCTLB = CHCTLB__R0CHCN__16_COLOR
                  | CHCTLB__R0BMEN__CELL_FORMAT
                  | CHCTLB__R0CHSZ__1x1_CELL;

  /* plane size */
  vdp2.reg.PLSZ = PLSZ__RAPLSZ__1x1
                | PLSZ__RBPLSZ__1x1;

  /* map plane offset
     1-word: value of bit 6-0 * 0x2000
     2-word: value of bit 5-0 * 0x4000
  */
  // plane_a_offset is at the start of VRAM-A1
  constexpr int plane_a = 8;
  constexpr int plane_a_offset = plane_a * 0x4000;

  constexpr int page_size = 64 * 64 * 2; // N0PNB__1WORD (16-bit)
  constexpr int plane_size = page_size * 1;

  /* cycle pattern table not used for RBG0 ? */
  vdp2.reg.CYCA0 = 0x0F44F99F;
  vdp2.reg.CYCA1 = 0x0F44F99F;
  vdp2.reg.CYCB0 = 0x0F44F99F;
  vdp2.reg.CYCB1 = 0x0F44F99F;

  vdp2.reg.MPOFR = MPOFR__RAMP(0); // bits 8~6
  vdp2.reg.MPABRA = MPABRA__RAMPB(plane_a) | MPABRA__RAMPA(plane_a); // bits 5~0
  vdp2.reg.MPCDRA = MPCDRA__RAMPD(plane_a) | MPCDRA__RAMPC(plane_a); // bits 5~0

  vdp2.reg.PNCR = PNCR__R0PNB__2WORD;

  vdp2.reg.RPMD = RPMD__ROTATION_PARAMETER_A;

  vdp2.reg.RPRCTL = 0;

  vdp2.reg.KTCTL = 0;

  vdp2.reg.KTAOF = 0;

  vdp2.reg.RPTA = 0;

  /* */

  palette_data();
  cell_data();

  for (int i = 0; i < 64 * 64; i++) {
    vdp2.vram.u32[(plane_a_offset / 4) + i] = ' ' - 0x20;
  }

  const char * test = "If sound level changes progressively, volume on 3 bit makes audi";
  int ix = 0;
  while (*test) {
    uint8_t c = *test++;
    vdp2.vram.u32[(plane_a_offset / 4) + ix++] = c - 0x20;
  }

  while (1);
}
