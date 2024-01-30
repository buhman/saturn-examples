#include <stdint.h>

#include "vdp2.h"
#include "smpc.h"
#include "scsp.h"
#include "scu.h"
#include "sh2.h"

#include "../common/copy.hpp"
#include "../common/vdp2_func.hpp"
#include "../common/string.hpp"

extern void * _nec_bitmap_start __asm("_binary_res_nec_bitmap_bin_start");

extern void * _m68k_start __asm("_binary_m68k_midi_debug_bin_start");
extern void * _m68k_size __asm("_binary_m68k_midi_debug_bin_size");

constexpr inline uint16_t rgb15(int32_t r, int32_t g, int32_t b)
{
  return ((b & 31) << 10) | ((g & 31) << 5) | ((r & 31) << 0);
}

void palette_data()
{
  vdp2.cram.u16[1 + 0 ] = rgb15( 0,  0,  0);
  vdp2.cram.u16[2 + 0 ] = rgb15(31, 31, 31);

  vdp2.cram.u16[1 + 16] = rgb15(31, 31, 31);
  vdp2.cram.u16[2 + 16] = rgb15( 0,  0,  0);

  vdp2.cram.u16[1 + 32] = rgb15(10, 10, 10);
  vdp2.cram.u16[2 + 32] = rgb15(31, 31, 31);
}

namespace pix_fmt_4bpp
{
  constexpr inline uint32_t
  bit(uint8_t n, int32_t i)
  {
    i &= 7;
    auto b = (n >> (7 - i)) & 1;
    return ((b + 1) << ((7 - i) * 4));
  }

  constexpr inline uint32_t
  bits(uint8_t n)
  {
    return
        bit(n, 0) | bit(n, 1) | bit(n, 2) | bit(n, 3)
      | bit(n, 4) | bit(n, 5) | bit(n, 6) | bit(n, 7);
  }

  static_assert(bits(0b1100'1110) == 0x2211'2221);
  static_assert(bits(0b1010'0101) == 0x2121'1212);
  static_assert(bits(0b1000'0000) == 0x2111'1111);
}

void cell_data()
{
  const uint8_t * normal = reinterpret_cast<uint8_t*>(&_nec_bitmap_start);

  for (int ix = 0; ix <= (0x7f - 0x20); ix++) {
    for (int y = 0; y < 8; y++) {
      const uint8_t row_n = normal[ix * 8 + y];
      vdp2.vram.u32[ 0 + (ix * 8) + y] = pix_fmt_4bpp::bits(row_n);
    }
  }
}

constexpr int32_t plane_a = 2;
constexpr inline int32_t plane_offset(int32_t n) { return n * 0x2000; }
constexpr int32_t page_size = 64 * 64 * 2; // N0PNB__1WORD (16-bit)
constexpr int32_t plane_size = page_size * 1;
constexpr int32_t page_width = 64;
static int plane_ix = 0;

void init_vdp2()
{
  v_blank_in();

  // DISP: Please make sure to change this bit from 0 to 1 during V blank.
  vdp2.reg.TVMD = ( TVMD__DISP | TVMD__LSMD__NON_INTERLACE
                  | TVMD__VRESO__240 | TVMD__HRESO__NORMAL_320);

  /* set the color mode to 5bits per channel, 1024 colors */
  vdp2.reg.RAMCTL = RAMCTL__CRMD__RGB_5BIT_1024;

  /* enable display of NBG0 */
  vdp2.reg.BGON = BGON__N0ON;

  /* set character format for NBG0 to palettized 16 color
     set enable "cell format" for NBG0
     set character size for NBG0 to 1x1 cell */
  vdp2.reg.CHCTLA = CHCTLA__N0CHCN__16_COLOR
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
  vdp2.reg.MPOFN = MPOFN__N0MP(0); // bits 8~6
  vdp2.reg.MPABN0 = MPABN0__N0MPB(plane_a) | MPABN0__N0MPA(plane_a); // bits 5~0
  vdp2.reg.MPCDN0 = MPCDN0__N0MPD(plane_a) | MPCDN0__N0MPC(plane_a); // bits 5~0

  // zeroize character/cell data from 0 up to plane_a_offset
  fill<uint32_t>(&vdp2.vram.u32[(0 / 4)], 0, plane_offset(plane_a));

  // zeroize plane_a; `0` is the ascii 0x20 ("space") which doubles as
  // "transparency" character.
  fill<uint32_t>(&vdp2.vram.u32[(plane_offset(plane_a) / 4)], 0, plane_size * 2);
}

void
set_char(int32_t x, int32_t y, uint8_t palette, uint8_t c)
{
  const auto ix = (plane_offset(plane_a + plane_ix) / 2) + (y * page_width) + x;
  vdp2.vram.u16[ix] =
      PATTERN_NAME_TABLE_1WORD__PALETTE(palette)
    | PATTERN_NAME_TABLE_1WORD__CHARACTER((c - 0x20));
}

static uint16_t * debug_buf = &scsp.ram.u16[0x080000 / 4];
static uint16_t * debug_length = &scsp.ram.u16[(0x080000 / 4) - 2];

void render()
{
  static uint8_t lbuf[4];
  string::hex(lbuf, 4, *debug_length);
  for (uint32_t i = 0; i < 4; i++) set_char(i, 0, 0, lbuf[i]);

  for (uint32_t i = 0; i < *debug_length; i++) {
    uint8_t buf[2];
    string::hex(buf, 2, debug_buf[i]);

    int32_t x = 1 + (i % 8);
    int32_t y = 1 + (i / 8);

    set_char(x * 3 + 0, y, 0, buf[0]);
    set_char(x * 3 + 1, y, 0, buf[1]);
  }
}

extern "C"
void v_blank_in_int(void) __attribute__ ((interrupt_handler));
void v_blank_in_int()
{
  scu.reg.IST &= ~(IST__V_BLANK_IN);
  scu.reg.IMS = ~(IMS__V_BLANK_IN);

  render();
}

void main()
{
  /* SEGA SATURN TECHNICAL BULLETIN # 51

     The document suggests that Sound RAM is (somewhat) preserved
     during SNDOFF.
   */

  while ((smpc.reg.SF & 1) != 0);
  smpc.reg.SF = 1;
  smpc.reg.COMREG = COMREG__SNDOFF;
  while (smpc.reg.OREG[31].val != OREG31__SNDOFF);

  scsp.reg.ctrl.MIXER = MIXER__MEM4MB;

  /*
    The Saturn BIOS does not (un)initialize the DSP. Without zeroizing the DSP
    program, the SCSP DSP appears to have a program that continuously writes to
    0x30000 through 0x3ffff in sound RAM, which has the effect of destroying any
    samples stored there.
  */
  reg32 * dsp_steps = reinterpret_cast<reg32*>(&(scsp.reg.dsp.STEP[0].MPRO[0]));
  fill<reg32>(dsp_steps, 0, (sizeof (scsp.reg.dsp.STEP)));

  uint32_t * m68k_main_start = reinterpret_cast<uint32_t*>(&_m68k_start);
  uint32_t m68k_main_size = reinterpret_cast<uint32_t>(&_m68k_size);
  copy<uint32_t>(&scsp.ram.u32[0], m68k_main_start, m68k_main_size);

  while ((smpc.reg.SF & 1) != 0);
  smpc.reg.SF = 1;
  smpc.reg.COMREG = COMREG__SNDON;
  while (smpc.reg.OREG[31].val != OREG31__SNDON);

  // do nothing while the sound CPU manipulates the SCSP

  init_vdp2();
  palette_data();
  cell_data();

  sh2_vec[SCU_VEC__V_BLANK_IN] = (u32)(&v_blank_in_int);

  scu.reg.IST = 0;
  scu.reg.IMS = ~(IMS__V_BLANK_IN);
}
