#include <stdint.h>

#include "smpc.h"
#include "scsp.h"
#include "vdp1.h"
#include "vdp2.h"
#include "scu.h"
#include "sh2.h"

#include "../common/copy.hpp"
#include "../common/font.hpp"
#include "../common/draw_font.hpp"
#include "../common/palette.hpp"
#include "../common/vdp2_func.hpp"

extern void * _m68k_start __asm("_binary_m68k_interrupt_bin_start");
extern void * _m68k_size __asm("_binary_m68k_interrupt_bin_size");
extern void * _sperrypc_start __asm("_binary_res_sperrypc_font_bin_start");

struct draw_state {
  uint32_t cmd_ix;
  struct draw_font::state font;
};

static struct draw_state draw_state;

uint32_t print_hex(uint8_t * c, uint32_t len, uint32_t n)
{
  uint32_t ret = 0;

  while (len > 0) {
    uint32_t nib = n & 0xf;
    n = n >> 4;

    if (nib > 9) {
      nib += (97 - 10);
    } else {
      nib += (48 - 0);
    }

    c[--len] = nib;

    ret++;
  }
  return ret;
}

int32_t draw_string(const uint8_t * string,
		    const uint32_t length,
		    const int32_t x,
		    const int32_t y)
{
  return draw_font::horizontal_string(draw_state.font,
				      draw_state.cmd_ix,
				      string,
				      length,
				       (x    ),
				      ((y + 1) * 8) << 6);
}

static inline uint32_t init_font(uint32_t top)
{
  // 256 is the number of colors in the color palette, not the number of grays
  // that are used by the font.
  constexpr uint32_t colors_per_palette = 256;
  constexpr uint32_t color_bank_index = 0; // completely random and arbitrary value

  palette::vdp2_cram_32grays(colors_per_palette, color_bank_index);
  // For color bank color, COLR is concatenated bitwise with pixel data. See
  // Figure 6.17 in the VDP1 manual.
  draw_state.font.color_address = color_bank_index << 8;

  top = font_data(&_sperrypc_start, top, draw_state.font);

  return top;
}

void init_sound()
{
  /* SEGA SATURN TECHNICAL BULLETIN # 51

     The document suggests that Sound RAM is (somewhat) preserved
     during SNDOFF.
   */

  while ((smpc.reg.SF & 1) != 0);
  smpc.reg.SF = 1;
  smpc.reg.COMREG = COMREG__SNDOFF;
  while (smpc.reg.oreg[31] != OREG31__SNDOFF);

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
  while (smpc.reg.oreg[31] != OREG31__SNDON);
}

static inline void init_vdp()
{
  // wait for the beginning of a V blank
  v_blank_in();

  // DISP: Please make sure to change this bit from 0 to 1 during V blank.
  vdp2.reg.TVMD = ( TVMD__DISP | TVMD__LSMD__NON_INTERLACE
                  | TVMD__VRESO__240 | TVMD__HRESO__NORMAL_320);

  // disable all VDP2 backgrounds (e.g: the Sega bios logo)
  vdp2.reg.BGON = 0;

  // zeroize BACK color
  vdp2.reg.BKTAU = 0;
  vdp2.reg.BKTAL = 0;
  vdp2.vram.u16[0] = 0;

  vdp2.reg.PRISA = PRISA__S0PRIN(1); // Sprite register 0 PRIority Number

  /* TVM settings must be performed from the second H-blank IN interrupt after the
  V-blank IN interrupt to the H-blank IN interrupt immediately after the V-blank
  OUT interrupt. */
  // "normal" display resolution, 16 bits per pixel, 512x256 framebuffer
  vdp1.reg.TVMR = TVMR__TVM__NORMAL;

  // swap framebuffers every 1 cycle; non-interlace
  vdp1.reg.FBCR = 0;

  // during a framebuffer erase cycle, write the color "black" to each pixel
  constexpr uint16_t black = 0x0000;
  vdp1.reg.EWDR = black;

  // erase upper-left coordinate
  vdp1.reg.EWLR = EWLR__16BPP_X1(0) | EWLR__Y1(0);

  // erase lower-right coordinate
  vdp1.reg.EWRR = EWRR__16BPP_X3(319) | EWRR__Y3(239);

  vdp1.vram.cmd[0].CTRL = CTRL__JP__JUMP_NEXT | CTRL__COMM__SYSTEM_CLIP_COORDINATES;
  vdp1.vram.cmd[0].LINK = 0;
  vdp1.vram.cmd[0].XC = 319;
  vdp1.vram.cmd[0].YC = 239;

  vdp1.vram.cmd[1].CTRL = CTRL__JP__JUMP_NEXT | CTRL__COMM__LOCAL_COORDINATE;
  vdp1.vram.cmd[1].LINK = 0;
  vdp1.vram.cmd[1].XA = 0;
  vdp1.vram.cmd[1].YA = 0;

  vdp1.vram.cmd[2].CTRL = CTRL__END;

  draw_state.cmd_ix = 2;

  // start drawing (execute the command list) on every frame
  vdp1.reg.PTMR = PTMR__PTM__FRAME_CHANGE;
}

template <uint32_t n, uint32_t size>
inline void draw_label(const uint8_t(&label)[size], int32_t advance, int32_t & row, uint32_t value)
{
  advance = (advance * 8) << 6;
  advance += draw_string(label, (sizeof (label)) - 1, advance, row);
  uint8_t v[n];
  print_hex(v, (sizeof (v)), value);
  draw_string(v, (sizeof (v)), advance, row);

  row++;  
};

static inline void render()
{
  draw_state.cmd_ix = 2;

  if ((scsp.ram.u32[0] & 0xffff0000) != static_cast<uint32_t>(0xdead << 16))
    return;

  uint16_t slot_ix = scsp.ram.u32[0] & 0xffff;
  uint16_t slotp_ix = (slot_ix - 1) & 31;
  slot_ix &= 31;
  uint32_t frame = scsp.ram.u32[1];

  int32_t row = 1;

  const uint8_t frame_l[] = "frame: 0x";
  draw_label<4>(frame_l, 1, row, frame);

  row++;

  // previous slot
  const uint8_t slotp_l[] = "slot_previous: 0x";
  draw_label<2>(slotp_l, 1, row, slotp_ix);

  scsp_slot& slot_p = scsp.reg.slot[slotp_ix];
  
  const uint8_t kyonb_l[] = "KYONB: ";  
  const uint32_t kyonb_p = (slot_p.LOOP >> 11) & 1;
  draw_label<1>(kyonb_l, 3, row, kyonb_p);

  row++;

  // current slot
  const uint8_t slot_l[] = "slot: 0x";
  draw_label<2>(slot_l, 1, row, slot_ix);

  scsp_slot& slot = scsp.reg.slot[slot_ix];

  uint32_t kyonb = (slot.LOOP >> 11) & 1;
  draw_label<1>(kyonb_l, 3, row, kyonb);

  const uint8_t sa_l[] = "SA: 0x";
  const uint32_t sa = ((slot.LOOP & 0b1111) << 16) | slot.SA;
  draw_label<5>(sa_l, 3, row, sa);

  const uint8_t lsa_l[] = "LSA: 0x";
  draw_label<4>(lsa_l, 3, row, slot.LSA);

  const uint8_t lea_l[] = "LEA: 0x";
  draw_label<4>(lea_l, 3, row, slot.LEA);    
  
  row++;

  // global
  const uint8_t ca_l[] = "STATUS__CA: 0x";
  draw_label<1>(ca_l, 1, row, STATUS__CA(scsp.reg.ctrl.STATUS));

  vdp1.vram.cmd[draw_state.cmd_ix].CTRL = CTRL__END;
}

void v_blank_in_int(void) __attribute__ ((interrupt_handler));
void v_blank_in_int()
{
  scu.reg.IST &= ~(IST__V_BLANK_IN);
  scu.reg.IMS = ~(IMS__V_BLANK_IN);

  render();
}

void main()
{
  uint32_t top = (sizeof (union vdp1_vram));
  top = init_font(top);

  init_vdp();

  sh2_vec[SCU_VEC__V_BLANK_IN] = (u32)(&v_blank_in_int);

  scu.reg.IST = 0;
  scu.reg.IMS = ~(IMS__V_BLANK_IN);

  init_sound();
}

extern "C"
void start(void)
{
  main();
  while (1);
}
