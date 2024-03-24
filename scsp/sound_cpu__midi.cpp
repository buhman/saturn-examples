#include <cstdint>
#include <optional>

#include "vdp2.h"
#include "smpc.h"
#include "scu.h"
#include "sh2.h"
#include "scsp.h"

#include "../common/copy.hpp"
#include "../common/intback.hpp"
#include "../common/vdp2_func.hpp"
#include "../common/string.hpp"

extern void * _m68k_start __asm("_binary_m68k_midi_bin_start");
extern void * _m68k_size __asm("_binary_m68k_midi_bin_size");

extern void * _nec_bitmap_start __asm("_binary_res_nec_bitmap_bin_start");

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

struct count_flop {
  s8 count;
  u8 flop;
  u8 das;
  u8 repeat;
};

struct input {
  count_flop right;
  count_flop left;
  count_flop down;
  count_flop up;
  count_flop start;
  count_flop a;
  count_flop b;
  count_flop c;
  count_flop r;
  count_flop x;
  count_flop y;
  count_flop z;
  count_flop l;
};

constexpr int input_arr = 10;
constexpr int input_das = 20;
constexpr int input_debounce = 2;

static inline void
input_count(count_flop& button, uint32_t input, uint32_t mask)
{
  if ((input & mask) == 0) {
    if (button.count < input_debounce)
      button.count += 1;
    else
      button.das += 1;
  } else {
    if (button.count == 0) {
      button.flop = 0;
      button.das = 0;
      button.repeat = 0;
    }
    else if (button.count > 0)
      button.count -= 1;
  }
}

static inline int32_t
input_flopped(count_flop& button)
{
  if (button.count == input_debounce && button.flop == 0) {
    button.flop = 1;
    return 1;
  } else if (button.flop == 1 && button.das == input_das && button.repeat == 0) {
    button.repeat = 1;
    button.das = 0;
    return 2;
  } else if (button.repeat == 1 && (button.das == input_arr)) {
    button.das = 0;
    return 2;
  } else {
    return 0;
  }
}

struct state {
  struct input input;
};

static struct state state = { 0 };

void digital_callback(uint8_t fsm_state, uint8_t data)
{
  switch (fsm_state) {
  case intback::DATA1:
    input_count(state.input.right, data, DIGITAL__1__RIGHT);
    input_count(state.input.left, data, DIGITAL__1__LEFT);
    input_count(state.input.down, data, DIGITAL__1__DOWN);
    input_count(state.input.up, data, DIGITAL__1__UP);
    input_count(state.input.start, data, DIGITAL__1__START);
    input_count(state.input.a, data, DIGITAL__1__A);
    input_count(state.input.c, data, DIGITAL__1__C);
    input_count(state.input.b, data, DIGITAL__1__B);
    break;
  case intback::DATA2:
    input_count(state.input.r, data, DIGITAL__2__R);
    input_count(state.input.x, data, DIGITAL__2__X);
    input_count(state.input.y, data, DIGITAL__2__Y);
    input_count(state.input.z, data, DIGITAL__2__Z);
    input_count(state.input.l, data, DIGITAL__2__L);
    break;
  default: break;
  }
}

extern "C"
void smpc_int(void) __attribute__ ((interrupt_handler));
void smpc_int(void)
{
  scu.reg.IST &= ~(IST__SMPC);
  scu.reg.IMS = ~(IMS__SMPC | IMS__V_BLANK_IN);

  intback::fsm(digital_callback, nullptr);
}

constexpr int32_t plane_a = 2;
constexpr inline int32_t plane_offset(int32_t n) { return n * 0x2000; }

constexpr int32_t page_size = 64 * 64 * 2; // N0PNB__1WORD (16-bit)
constexpr int32_t plane_size = page_size * 1;

constexpr int32_t cell_size = (8 * 8) / 2; // N0CHCN__16_COLOR (4-bit)
constexpr int32_t character_size = cell_size * (1 * 1); // N0CHSZ__1x1_CELL
constexpr int32_t page_width = 64;

static int plane_ix = 0;

inline void
set_char(int32_t x, int32_t y, uint8_t palette, uint8_t c)
{
  const auto ix = (plane_offset(plane_a + plane_ix) / 2) + (y * page_width) + x;
  vdp2.vram.u16[ix] =
      PATTERN_NAME_TABLE_1WORD__PALETTE(palette)
    | PATTERN_NAME_TABLE_1WORD__CHARACTER((c - 0x20));
}

static int render_state = 0;

void render()
{
  if (++render_state < 1)
    return;
  render_state = 0;

  //                 012345678901234
  static uint8_t label[] = "midi";
  for (uint32_t i = 0; label[i] != 0; i++) {
    set_char(0 + i, 1, 0, label[i]);
  }

  uint32_t i;
  uint8_t buf[8];
  static uint8_t l1[] = "evnt";
  string::hex(buf, 8, scsp.ram.u32[0]);
  for (i = 0; i < 4; i++) set_char(0 + i, 3, 0, l1[i]);
  for (i = 0; i < 8; i++) set_char(5 + i, 3, 0, buf[i]);
  static uint8_t l2[] = "s_dt";
  string::hex(buf, 8, scsp.ram.u32[1]);
  for (i = 0; i < 4; i++) set_char(0 + i, 4, 0, l2[i]);
  for (i = 0; i < 8; i++) set_char(5 + i, 4, 0, buf[i]);
  static uint8_t l3[] = "e_dt";
  string::hex(buf, 8, scsp.ram.u32[2]);
  for (i = 0; i < 4; i++) set_char(0 + i, 5, 0, l3[i]);
  for (i = 0; i < 8; i++) set_char(5 + i, 5, 0, buf[i]);
  static uint8_t l4[] = "note";
  string::hex(buf, 8, scsp.ram.u32[3]);
  for (i = 0; i < 4; i++) set_char(0 + i, 6, 0, l4[i]);
  for (i = 0; i < 8; i++) set_char(5 + i, 6, 0, buf[i]);
}

void update()
{
}

extern "C"
void v_blank_in_int(void) __attribute__ ((interrupt_handler));
void v_blank_in_int()
{
  scu.reg.IST &= ~(IST__V_BLANK_IN);
  scu.reg.IMS = ~(IMS__SMPC | IMS__V_BLANK_IN);

  // flip planes;
  vdp2.reg.MPCDN0 = MPCDN0__N0MPB(0) | MPCDN0__N0MPA(plane_a + plane_ix);
  //plane_ix = !plane_ix;

  // wait at least 300us, as specified in the SMPC manual.
  // It appears reading FRC.H is mandatory and *must* occur before FRC.L on real
  // hardware.
  while ((sh2.reg.FTCSR & FTCSR__OVF) == 0 && sh2.reg.FRC.H == 0 && sh2.reg.FRC.L < 63);

  if ((vdp2.reg.TVSTAT & TVSTAT__VBLANK) != 0) {
    // on real hardware, SF contains uninitialized garbage bits other than the
    // lsb.
    while ((smpc.reg.SF & 1) != 0);

    smpc.reg.SF = 0;

    smpc.reg.IREG[0].val = INTBACK__IREG0__STATUS_DISABLE;
    smpc.reg.IREG[1].val = ( INTBACK__IREG1__PERIPHERAL_DATA_ENABLE
                           | INTBACK__IREG1__PORT2_15BYTE
                           | INTBACK__IREG1__PORT1_15BYTE
                           );
    smpc.reg.IREG[2].val = INTBACK__IREG2__MAGIC;

    smpc.reg.COMREG = COMREG__INTBACK;
  }

  update();
  render();
}

void init_snd_cpu()
{
  /*
    The Saturn BIOS does not (un)initialize the DSP. Without zeroizing the DSP
    program, the SCSP DSP appears to have a program that continuously writes to
    0x30000 through 0x3ffff in sound RAM, which has the effect of destroying any
    samples stored there.
  */
  reg32 * dsp_steps = reinterpret_cast<reg32*>(&(scsp.reg.dsp.STEP[0].MPRO[0]));
  fill<reg32>(dsp_steps, 0, (sizeof (scsp.reg.dsp.STEP)));

  /* SEGA SATURN TECHNICAL BULLETIN # 51

     The document suggests that Sound RAM is (somewhat) preserved
     during SNDOFF.
   */

  scsp.reg.ctrl.MIXER = MIXER__MEM4MB;

  while ((smpc.reg.SF & 1) != 0);
  smpc.reg.SF = 1;
  smpc.reg.COMREG = COMREG__SNDOFF;
  while (smpc.reg.OREG[31].val != OREG31__SNDOFF);

  uint32_t * m68k_main_start = reinterpret_cast<uint32_t*>(&_m68k_start);
  uint32_t m68k_main_size = reinterpret_cast<uint32_t>(&_m68k_size);
  copy<uint32_t>(&scsp.ram.u32[0], m68k_main_start, m68k_main_size);

  while ((smpc.reg.SF & 1) != 0);
  smpc.reg.SF = 1;
  smpc.reg.COMREG = COMREG__SNDON;
  while (smpc.reg.OREG[31].val != OREG31__SNDON);

  for (int i = 0; i < 32; i++) {
    break;
    scsp_slot& slot = scsp.reg.slot[i];
    // start address (bytes)
    slot.SA = 0; // kx kb sbctl[1:0] ssctl[1:0] lpctl[1:0] 8b sa[19:0]
    slot.LSA = 0; // loop start address (samples)
    slot.LEA = 0; // loop end address (samples)
    slot.EG = EG__RR(0x1f); // d2r d1r ho ar krs dl rr
    slot.FM = 0; // stwinh sdir tl mdl mdxsl mdysl
    slot.PITCH = 0; // oct fns
    slot.LFO = 0; // lfof plfows
    slot.MIXER = 0; // disdl dipan efsdl efpan
  }
  //scsp.reg.slot[0].LOOP |= LOOP__KYONEX;

  scsp.reg.ctrl.MIXER = MIXER__MEM4MB | MIXER__MVOL(0xf);
}

void main()
{
  init_snd_cpu();

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
  vdp2.reg.MPCDN0 = MPCDN0__N0MPB(0) | MPCDN0__N0MPA(plane_a); // bits 5~0
  vdp2.reg.MPCDN0 = MPCDN0__N0MPD(0) | MPCDN0__N0MPC(0); // bits 5~0

  // zeroize character/cell data from 0 up to plane_a_offset
  fill<uint32_t>(&vdp2.vram.u32[(0 / 4)], 0, plane_offset(plane_a));

  // zeroize plane_a; `0` is the ascii 0x20 ("space") which doubles as
  // "transparency" character.
  fill<uint32_t>(&vdp2.vram.u32[(plane_offset(plane_a) / 4)], 0, plane_size * 2);

  palette_data();
  cell_data();

  // free-running timer
  sh2.reg.TCR = TCR__CKS__INTERNAL_DIV128;
  sh2.reg.FTCSR = 0;

  // initialize smpc
  smpc.reg.DDR1 = 0; // INPUT
  smpc.reg.DDR2 = 0; // INPUT
  smpc.reg.IOSEL = 0; // SMPC control
  smpc.reg.EXLE = 0; //

  sh2_vec[SCU_VEC__SMPC] = (u32)(&smpc_int);
  sh2_vec[SCU_VEC__V_BLANK_IN] = (u32)(&v_blank_in_int);

  scu.reg.IST = 0;
  scu.reg.IMS = ~(IMS__SMPC | IMS__V_BLANK_IN);
}
