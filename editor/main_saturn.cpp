#include <stdint.h>

#include "vdp2.h"
#include "smpc.h"
#include "scu.h"
#include "sh2.h"

#include "../common/copy.hpp"
#include "../common/vdp2_func.hpp"
#include "../common/keyboard.hpp"
#include "../common/intback.hpp"

#include "editor.hpp"

extern void * _sperrypc_bitmap_start __asm("_binary_res_sperrypc_bitmap_bin_start");

using buffer_type = editor::buffer<64, 64>;
constexpr int32_t viewport_max_col = 320 / 8;
constexpr int32_t viewport_max_row = 240 / 8;

static buffer_type buffer {viewport_max_col, viewport_max_row};

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
  const uint8_t * buf = reinterpret_cast<uint8_t*>(&_sperrypc_bitmap_start);

  for (int ix = 0; ix <= (0x7f - 0x20); ix++) {
    for (int y = 0; y < 8; y++) {
      uint8_t row = buf[ix * 8 + y];
      vdp2.vram.u32[(ix * 8) + y] = pix_fmt_4bpp::bits(row);
    }
  }
}

enum modifier {
  MODIFIER_NONE = 0,
  MODIFIER_LEFT_SHIFT = (1 << 0),
  MODIFIER_RIGHT_SHIFT = (1 << 1),
  MODIFIER_BOTH_SHIFT = MODIFIER_LEFT_SHIFT | MODIFIER_RIGHT_SHIFT,
  MODIFIER_LEFT_CTRL = (1 << 2),
  MODIFIER_RIGHT_CTRL = (1 << 3),
  MODIFIER_BOTH_CTRL = MODIFIER_LEFT_CTRL | MODIFIER_RIGHT_CTRL,
  MODIFIER_LEFT_ALT = (1 << 4),
  MODIFIER_RIGHT_ALT = (1 << 5),
  MODIFIER_BOTH_ALT = MODIFIER_LEFT_ALT | MODIFIER_RIGHT_ALT,
};

static uint32_t modifier_state = 0;

inline void keyboard_regular_key(const enum keysym k)
{
  switch (modifier_state) {
  case MODIFIER_NONE:
    switch (k) {
    case keysym::BACKSPACE  : buffer.delete_backward(); break;
    case keysym::DELETE     : buffer.delete_forward(); break;
    case keysym::ARROW_LEFT : buffer.cursor_left(); break;
    case keysym::ARROW_RIGHT: buffer.cursor_right(); break;
    case keysym::ARROW_UP   : buffer.cursor_up(); break;
    case keysym::ARROW_DOWN : buffer.cursor_down(); break;
    case keysym::HOME       : buffer.cursor_home(); break;
    case keysym::END        : buffer.cursor_end(); break;
    case keysym::ENTER      : buffer.enter(); break;
    default:
      {
	int32_t c = keysym_to_char(k, false);
	if (c != -1) buffer.put(c);
      }
      break;
    }
    break;

  case MODIFIER_LEFT_SHIFT: [[fallthrough]];
  case MODIFIER_RIGHT_SHIFT: [[fallthrough]];
  case MODIFIER_BOTH_SHIFT:
    switch (k) {
    default:
      {
	int32_t c = keysym_to_char(k, true);
	if (c != -1) buffer.put(c);
      }
      break;
    }
    break;

  case MODIFIER_LEFT_CTRL: [[fallthrough]];
  case MODIFIER_RIGHT_CTRL: [[fallthrough]];
  case MODIFIER_BOTH_CTRL:
    switch (k) {
    case keysym::SPACE: buffer.mark_set(); break;
    case keysym::G    : buffer.quit(); break;
    case keysym::B    : buffer.cursor_left(); break;
    case keysym::F    : buffer.cursor_right(); break;
    case keysym::P    : buffer.cursor_up(); break;
    case keysym::N    : buffer.cursor_down(); break;
    case keysym::A    : buffer.cursor_home(); break;
    case keysym::E    : buffer.cursor_end(); break;
    case keysym::Y    : buffer.shadow_paste(); break;
    case keysym::W    : buffer.shadow_cut(); break;
    default: break;
    }
    break;

  case MODIFIER_LEFT_ALT: [[fallthrough]];
  case MODIFIER_RIGHT_ALT: [[fallthrough]];
  case MODIFIER_BOTH_ALT:
    switch (k) {
    case keysym::ARROW_LEFT : buffer.cursor_scan_word_backward(); break;
    case keysym::B          : buffer.cursor_scan_word_backward(); break;
    case keysym::ARROW_RIGHT: buffer.cursor_scan_word_forward(); break;
    case keysym::F          : buffer.cursor_scan_word_forward(); break;
    case keysym::W          : buffer.shadow_copy(); break;
    default: break;
    }
    break;

  default: break;
  }
}

void keyboard_callback(const enum keysym k, uint8_t kbd_bits)
{
  if (KEYBOARD__MAKE(kbd_bits)) {
    switch (k) {
    case keysym::LEFT_SHIFT : modifier_state |= MODIFIER_LEFT_SHIFT; break;
    case keysym::RIGHT_SHIFT: modifier_state |= MODIFIER_RIGHT_SHIFT; break;
    case keysym::LEFT_CTRL  : modifier_state |= MODIFIER_LEFT_CTRL; break;
    case keysym::RIGHT_CTRL : modifier_state |= MODIFIER_RIGHT_CTRL; break;
    case keysym::LEFT_ALT   : modifier_state |= MODIFIER_LEFT_ALT; break;
    case keysym::RIGHT_ALT  : modifier_state |= MODIFIER_RIGHT_ALT; break;
    default:
      keyboard_regular_key(k);
      break;
    }
  } else if (KEYBOARD__BREAK(kbd_bits)) {
    switch (k) {
    case keysym::LEFT_SHIFT : modifier_state &= ~(MODIFIER_LEFT_SHIFT); break;
    case keysym::RIGHT_SHIFT: modifier_state &= ~(MODIFIER_RIGHT_SHIFT); break;
    case keysym::LEFT_CTRL  : modifier_state &= ~(MODIFIER_LEFT_CTRL); break;
    case keysym::RIGHT_CTRL : modifier_state &= ~(MODIFIER_RIGHT_CTRL); break;
    case keysym::LEFT_ALT   : modifier_state &= ~(MODIFIER_LEFT_ALT); break;
    case keysym::RIGHT_ALT  : modifier_state &= ~(MODIFIER_RIGHT_ALT); break;
    default: break;
    }
  }
}

extern "C"
void smpc_int(void) __attribute__ ((interrupt_handler));
void smpc_int(void)
{
  scu.reg.IST &= ~(IST__SMPC);
  scu.reg.IMS = ~(IMS__SMPC | IMS__V_BLANK_IN);

  intback::keyboard_fsm(keyboard_callback);
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

void render()
{
  editor::cursor& cur = buffer.cursor;

  editor::selection sel;

  if (buffer.mode == editor::mode::mark)
    sel = buffer.mark_get();

  for (int y = 0; y < buffer.window.cell_height; y++) {
    const int32_t row = buffer.window.top + y;
    const buffer_type::line_type * l = buffer.lines[row];

    for (int x = 0; x < buffer.window.cell_width; x++) {
      const int32_t col = buffer.window.left + x;
      const uint8_t c = (l != nullptr && col < l->length) ? l->buf[col] : ' ';
      uint8_t palette = 0;

      if (row == cur.row && col == cur.col) {
	palette = 1;
      } else if (buffer.mode == editor::mode::mark) {
	if (sel.contains(col, row))
	  palette = 2;
      }

      set_char(x, y, palette, c);
    }
  }
}

extern "C"
void v_blank_in_int(void) __attribute__ ((interrupt_handler));
void v_blank_in_int()
{
  scu.reg.IST &= ~(IST__V_BLANK_IN);
  scu.reg.IMS = ~(IMS__SMPC | IMS__V_BLANK_IN);

  // flip planes;
  vdp2.reg.MPABN0 = MPABN0__N0MPB(0) | MPABN0__N0MPA(plane_a + plane_ix);
  plane_ix = !plane_ix;

  // wait at least 300us, as specified in the SMPC manual.
  // It appears reading FRC.H is mandatory and *must* occur before FRC.L on real
  // hardware.
  while ((sh2.reg.FTCSR & FTCSR__OVF) == 0 && sh2.reg.FRC.H == 0 && sh2.reg.FRC.L < 63);

  if ((vdp2.reg.TVSTAT & TVSTAT__VBLANK) != 0) {
    // on real hardware, SF contains uninitialized garbage bits other than the
    // lsb.
    while ((smpc.reg.SF & 1) != 0);

    smpc.reg.SF = 0;

    smpc.reg.ireg[0] = INTBACK__IREG0__STATUS_DISABLE;
    smpc.reg.ireg[1] = ( INTBACK__IREG1__PERIPHERAL_DATA_ENABLE
                       | INTBACK__IREG1__PORT2_15BYTE
                       | INTBACK__IREG1__PORT1_15BYTE
                       );
    smpc.reg.ireg[2] = INTBACK__IREG2__MAGIC;

    smpc.reg.COMREG = COMREG__INTBACK;
  }

  render();
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
  vdp2.reg.MPABN0 = MPABN0__N0MPB(0) | MPABN0__N0MPA(plane_a); // bits 5~0
  vdp2.reg.MPCDN0 = MPABN0__N0MPD(0) | MPABN0__N0MPC(0); // bits 5~0

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
