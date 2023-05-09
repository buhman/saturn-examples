#include <stdint.h>
#include "vdp1.h"
#include "vdp2.h"
#include "smpc.h"
#include "sh2.h"
#include "scu.h"

#include "../common/keyboard.hpp"
#include "../common/font.hpp"

/* begin font */

extern void * _dejavusans_start __asm("_binary_res_dejavusansmono_font_bin_start");

constexpr inline uint16_t rgb15_gray(uint32_t intensity)
{
  return ((intensity & 31) << 10) // blue
       | ((intensity & 31) << 5 )  // green
       | ((intensity & 31) << 0 ); // red
}

void vdp2_color_palette(uint32_t colors, uint32_t color_bank)
{
  /* generate a palette of 32 grays */

  uint16_t * table = &vdp2.cram.u16[colors * color_bank];

  for (uint32_t i = 0; i <= 31; i++) {
    table[i] = rgb15_gray(i);
  }
}

template <typename T>
void copy(T * dst, const T * src, int32_t n) noexcept
{
  while (n > 0) {
    *dst++ = *src++;
    n -= (sizeof (T));
  }
}

struct color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

uint32_t pixel_data(const uint32_t top, const uint8_t * glyph_bitmaps, const uint32_t bitmap_offset)
{
  const uint32_t * buf = reinterpret_cast<const uint32_t *>(&glyph_bitmaps[0]);

  const uint32_t table_size = (bitmap_offset + 0x20 - 1) & (-0x20);
  const uint32_t table_address = top - table_size;

  uint32_t * table = &vdp1.vram.u32[(table_address / 4)];

  copy<uint32_t>(table, buf, bitmap_offset);

  return table_address;
}

uint32_t font_data(uint32_t top, const font ** font_out, const glyph ** glyphs_out)
{
  uint8_t * data = reinterpret_cast<uint8_t*>(&_dejavusans_start);

  const font * font = reinterpret_cast<struct font*>(&data[0]);
  const glyph * glyphs = reinterpret_cast<struct glyph*>(&data[(sizeof (struct font))]);
  // there are three 32-bit fields before the start of `glyphs`
  const uint8_t * glyph_bitmaps = &data[(sizeof (struct font)) + ((sizeof (struct glyph)) * font->glyph_index)];

  top = pixel_data(top, glyph_bitmaps, font->bitmap_offset);

  *font_out = font;
  *glyphs_out = glyphs;

  return top;
}

uint32_t draw_utf16_string(const uint32_t color_address,
			   const uint32_t character_address,
                           const uint32_t char_code_offset,
			   const glyph * glyphs,
			   uint32_t cmd_ix,
			   const char16_t * string,
			   const uint32_t length,
                           int32_t& x,
                           int32_t& y)
{
  for (uint32_t i = 0; i < length; i++) {
    const char16_t c = string[i];
    //assert(c <= char_code_offset);
    const uint16_t c_offset = c - char_code_offset;

    const glyph_bitmap& bitmap = glyphs[c_offset].bitmap;
    const glyph_metrics& metrics = glyphs[c_offset].metrics;

    if (bitmap.pitch != 0) {
      vdp1.vram.cmd[cmd_ix].CTRL = CTRL__JP__JUMP_NEXT | CTRL__COMM__NORMAL_SPRITE;
      vdp1.vram.cmd[cmd_ix].LINK = 0;
      vdp1.vram.cmd[cmd_ix].PMOD = PMOD__ECD | PMOD__COLOR_MODE__COLOR_BANK_256;
      vdp1.vram.cmd[cmd_ix].COLR = color_address; // non-palettized (rgb15) color data
      vdp1.vram.cmd[cmd_ix].SRCA = SRCA(character_address + bitmap.offset);
      vdp1.vram.cmd[cmd_ix].SIZE = SIZE__X(bitmap.pitch) | SIZE__Y(bitmap.rows);
      vdp1.vram.cmd[cmd_ix].XA = (x + metrics.horiBearingX) >> 6;
      vdp1.vram.cmd[cmd_ix].YA = (y - metrics.horiBearingY) >> 6;
      cmd_ix++;
    }

    x += metrics.horiAdvance;
  }

  return cmd_ix;
}

/* end font */

/* square stuff */

const static color colors[16] = {
  {255, 255, 255}, // (SPD / transparent) 0
  {0, 255, 255},   // I (cyan) 1
  {255, 255, 0},   // O (yellow) 2
  {128, 0, 128},   // T (purple) 3
  {0, 255, 0},     // S (green) 4
  {0, 0, 255},     // J (blue) 5
  {255, 0, 0},     // Z (red) 6
  {255, 128, 0},   // L (orange) 7
  {0},             // 8
  {0},             // 9
  {0},             // 10 (a)
  {0},             // 11 (b)
  {0},             // 12 (c)
  {0},             // 13 (d)
  {0},             // 14 (e)
  {250, 128, 114}  // (ECD) f
};

inline constexpr uint16_t rgb15(const color& color)
{
  return ((color.b >> 3) << 10) // blue
       | ((color.g >> 3) << 5 )  // green
       | ((color.r >> 3) << 0 ); // red
}

uint32_t color_lookup_table(const uint32_t top)
{
  // "The size of a color lookup table is 20H (32) bytes"
  // (assume top is already aligned to 0x20)
  const uint32_t table_address = top - 0x20;

  // "The color lookup table defines the respective color codes of 16 colors in
  // VRAM as 16-bit data"
  uint16_t * table = &vdp1.vram.u16[(table_address / 2)];

  uint32_t buf_ix = 0;
  for (uint32_t i = 0; i < 16; i++) {
    // there is a typo in "5.2 Color Lookup Tables" "If RGB code, MSB = 0"
    // should be "MSB = 1". The "MSB = 0" claim is correctly contradicted later.

    table[i] = 1 << 15 | rgb15(colors[i]);
    // _mai_data_pal is rgb24, 3 bytes per color
    buf_ix += 3;
  }

  return table_address;
}

template <class T>
inline constexpr T round(const T n)
{
  return (n + 0x20 - 1) & (-0x20);
}

constexpr uint32_t sprite_stride = 16;
constexpr uint32_t sprite_height = 10;
constexpr uint32_t sprite_width = 10;

uint32_t character_pattern_table(const uint32_t top, int color)
{
  constexpr uint32_t canvas_size = sprite_stride * 10;
  // 1 pixel = 4 bits
  constexpr uint32_t table_size = round(canvas_size / 2);
  const uint32_t table_address = top - table_size;

  uint32_t * table = &vdp1.vram.u32[(table_address / 4)];

  // `table_size` is in bytes; divide by two to get uint16_t indicies.
  uint32_t table_ix = 0;
  for (uint32_t row = 0; row < sprite_height; row++) {
    table[table_ix++] =
        (color << 28) | (color << 24) | (color << 20) | (color << 16)
      | (color << 12) | (color << 8 ) | (color << 4 ) | (color << 0 );

    // 0xf is the "end code" for 4-bit row/pixel data; there must be two
    // bit-consecutive codes for vdp2 to interpret it as an end code.
    constexpr int ecd = 0xf;
    table[table_ix++] =
        (color << 28) | (color << 24) | (ecd << 20) | (ecd << 16);
  }

  return table_address;
}

struct controller_state {
  uint8_t up;
  uint8_t down;
  uint8_t left;
  uint8_t right;
};

#define assert(n) if ((n) == 0) while (1);

enum intback_fsm {
  PORT_STATUS = 0,
  PERIPHERAL_ID,
  DATA1,
  DATA2,
  DATA3,
  DATA4,
  FSM_NEXT
};

struct intback_state {
  int fsm;
  int controller_ix;
  int port_ix;
  controller_state controller[2];
};

static intback_state intback;
static int oreg_ix;

struct xy {
  int x;
  int y;
};

static xy foo[2] = {
  {100, 100},
  {200, 100}
};

uint32_t print_hex(char16_t * c, uint32_t len, uint32_t n)
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

static uint32_t font_color_address, font_character_address;
static const font * font;
static const glyph * glyphs;

static uint32_t global_cmd_ix = 0;

void smpc_int(void) __attribute__ ((interrupt_handler));
void smpc_int(void) {
  scu.reg.IST &= ~(IST__SMPC);
  scu.reg.IMS = ~(IMS__SMPC | IMS__V_BLANK_IN);

  if ((smpc.reg.SR & SR__PDL) != 0) {
    // PDL == 1; 1st peripheral data
    oreg_ix = 0;
    intback.controller_ix = 0;
    intback.port_ix = 0;
    intback.fsm = PORT_STATUS;
  }

  int port_connected = 0;
  int data_size = 0;
  int peripheral_type;
  (void)peripheral_type;

  int kbd_bits = 0;

  /*
    This intback handling is oversimplified:

    - up to 2 controllers may be connected
    - controller port 1 must be connected (could relax this)
    - both controllers must be "digital pad" controllers
   */
  while (oreg_ix < 31) {
    reg8 const& oreg = smpc.reg.oreg[oreg_ix++];
    switch (intback.fsm++) {
    case PORT_STATUS:
      port_connected = (PORT_STATUS__CONNECTORS(oreg) == 1);
      if (port_connected) {
        assert(PORT_STATUS__MULTITAP_ID(oreg) == 0xf);
      } else {
        intback.fsm = FSM_NEXT;
      }
      break;
    case PERIPHERAL_ID:
      peripheral_type = PERIPHERAL_ID__TYPE(oreg);
      data_size = PERIPHERAL_ID__DATA_SIZE(oreg);
      break;
    case DATA1:
      {
        controller_state& c = intback.controller[intback.controller_ix];
        c.right = (DIGITAL__1__RIGHT & oreg) == 0;
        c.left  = (DIGITAL__1__LEFT  & oreg) == 0;
        c.down  = (DIGITAL__1__DOWN  & oreg) == 0;
        c.up    = (DIGITAL__1__UP    & oreg) == 0;
      }
      break;
    case DATA2:
      break;
    case DATA3:
      kbd_bits = oreg & 0b1111;
      break;
    case DATA4:
      {
        char16_t str_num[2];

        uint32_t keysym = oreg;
        char16_t text[1];

        static int32_t x = 8 << 6;
        static int32_t y = 50 << 6;

        if (kbd_bits & 0b1000) {
          print_hex(str_num, 2, keysym);

          enum keysym k = scancode_to_keysym(keysym);
          char16_t c = keysym_to_char16(k);
          if (k != keysym::UNKNOWN && c != -1) {
            text[0] = c;
            global_cmd_ix = draw_utf16_string(font_color_address,
                                              font_character_address,
                                              font->char_code_offset,
                                              glyphs,
                                              global_cmd_ix,
                                              &text[0],
                                              1,
                                              x,
                                              y);

            vdp1.vram.cmd[global_cmd_ix].CTRL = CTRL__END;
          }

        } else if (kbd_bits & 0b0001) {
          str_num[0] = 'q';
          str_num[1] = 'q';
        }

        int32_t qx = 8 << 6;
        int32_t qy = 150 << 6;
        uint32_t cmd_ix = 4;

        cmd_ix = draw_utf16_string(font_color_address,
                                   font_character_address,
                                   font->char_code_offset,
                                   glyphs,
                                   cmd_ix,
                                   str_num,
                                   2,
                                   qx,
                                   qy);
      }
      break;
    default:
      assert(0);
      break;
    }

    if ((intback.fsm >= PERIPHERAL_ID && (data_size--) < 0) || intback.fsm == FSM_NEXT) {
      if (intback.port_ix == 1)
        break;
      else {
        intback.port_ix++;
        intback.controller_ix++;
        intback.fsm = PORT_STATUS;
      }
    }
  }

  if ((smpc.reg.SR & SR__NPE) != 0) {
    smpc.reg.ireg[0] = INTBACK__IREG0__CONTINUE;
  } else {
    smpc.reg.ireg[0] = INTBACK__IREG0__BREAK;
  }
}

void v_blank_in_int(void) __attribute__ ((interrupt_handler));
void v_blank_in_int() {
  scu.reg.IST &= ~(IST__V_BLANK_IN);
  scu.reg.IMS = ~(IMS__SMPC | IMS__V_BLANK_IN);

  sh2.reg.FRC.H = 0;
  sh2.reg.FRC.L = 0;
  sh2.reg.FTCSR = 0; // clear flags

  for (int i = 0; i < 2; i++) {
    const controller_state& c = intback.controller[i];
    if (c.left)  vdp1.vram.cmd[2 + i].XA = --foo[i].x;
    if (c.right) vdp1.vram.cmd[2 + i].XA = ++foo[i].x;
    if (c.up)    vdp1.vram.cmd[2 + i].YA = --foo[i].y;
    if (c.down)  vdp1.vram.cmd[2 + i].YA = ++foo[i].y;
  }

  // wait 300us, as specified in the SMPC manual.
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
}

static inline void v_blank_in() {
  /*
     v
         _____
    ____|     |____

   */
  while ((vdp2.reg.TVSTAT & TVSTAT__VBLANK) != 0);
  while ((vdp2.reg.TVSTAT & TVSTAT__VBLANK) == 0);
}

void main()
{
  uint32_t top = (sizeof (union vdp1_vram));

  // begin font

  // 256 is the number of colors in the color palette, not the number of grays
  // that are used by the font.
  constexpr uint32_t font_colors = 256;
  constexpr uint32_t font_color_bank = 0; // completely random and arbitrary value

  vdp2_color_palette(font_colors, font_color_bank);
  // For color bank color, COLR is concatenated bitwise with pixel data. See
  // Figure 6.17 in the VDP1 manual.
  font_color_address = font_color_bank << 8;

  top = font_character_address = font_data(top, &font, &glyphs);

  // end font


  // begin squares

  uint32_t color_address, character_address[16];
  top = color_address     = color_lookup_table(top);
  top = character_address[0] = character_pattern_table(top, 1);
  top = character_address[1] = character_pattern_table(top, 2);

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

  // VDP2 User's Manual:
  // "When sprite data is in an RGB format, sprite register 0 is selected"
  // "When the value of a priority number is 0h, it is read as transparent"
  //
  // From a VDP2 perspective: in VDP1 16-color lookup table mode, VDP1 is still
  // sending RGB data to VDP2. This sprite color data as configured in
  // `color_lookup_table` from a VDP2 priority perspective uses sprite register 0.
  //
  // The power-on value of PRISA is zero. Set the priority for sprite register 0
  // to some number greater than zero, so that the color data is not interpreted
  // as "transparent".
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

  // the EWLR/EWRR macros use somewhat nontrivial math for the X coordinates
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

  vdp1.vram.cmd[2].CTRL = CTRL__JP__JUMP_NEXT | CTRL__COMM__NORMAL_SPRITE;
  vdp1.vram.cmd[2].LINK = 0;
  // The "end code" is 0xf, which is being used in the mai sprite palette. If
  // both transparency and end codes are enabled, it seems there are only 14
  // usable colors in the 4-bit color mode.
  vdp1.vram.cmd[2].PMOD = PMOD__COLOR_MODE__LOOKUP_TABLE_16;
  // It appears Kronos does not correctly calculate the color address in the
  // VDP1 debugger. Kronos will report FFFC when the actual color table address
  // in this example is 7FFE0.
  vdp1.vram.cmd[2].COLR = COLR__ADDRESS(color_address); // non-palettized (rgb15) color data
  vdp1.vram.cmd[2].SRCA = SRCA(character_address[0]);
  vdp1.vram.cmd[2].SIZE = SIZE__X(sprite_stride) | SIZE__Y(sprite_height);
  vdp1.vram.cmd[2].XA = foo[0].x;
  vdp1.vram.cmd[2].YA = foo[0].y;

  vdp1.vram.cmd[3].CTRL = CTRL__JP__JUMP_NEXT | CTRL__COMM__NORMAL_SPRITE;
  vdp1.vram.cmd[3].LINK = 0;
  // The "end code" is 0xf, which is being used in the mai sprite palette. If
  // both transparency and end codes are enabled, it seems there are only 14
  // usable colors in the 4-bit color mode.
  vdp1.vram.cmd[3].PMOD = PMOD__COLOR_MODE__LOOKUP_TABLE_16;
  // It appears Kronos does not correctly calculate the color address in the
  // VDP1 debugger. Kronos will report FFFC when the actual color table address
  // in this example is 7FFE0.
  vdp1.vram.cmd[3].COLR = COLR__ADDRESS(color_address); // non-palettized (rgb15) color data
  vdp1.vram.cmd[3].SRCA = SRCA(character_address[1]);
  vdp1.vram.cmd[3].SIZE = SIZE__X(sprite_stride) | SIZE__Y(sprite_height);
  vdp1.vram.cmd[3].XA = foo[1].x;
  vdp1.vram.cmd[3].YA = foo[1].y;

  int32_t qx = 8 << 6;
  int32_t qy = 150 << 6;

  uint32_t cmd_ix = 4;
  const char16_t _xx[] = u"xx";
  cmd_ix = draw_utf16_string(font_color_address,
			     font_character_address,
                             font->char_code_offset,
			     glyphs,
			     cmd_ix,
			     _string,
			     ((sizeof (_xx)) / (sizeof (_xx[0]))) - 1,
                             qx,
                             qy);

  vdp1.vram.cmd[cmd_ix].CTRL = CTRL__END;
  global_cmd_ix = cmd_ix;

  // start drawing (execute the command list) on every frame
  vdp1.reg.PTMR = PTMR__PTM__FRAME_CHANGE;

  // free-running timer
  sh2.reg.TCR = TCR__CKS__INTERNAL_DIV128;
  sh2.reg.FTCSR = 0;

  // initialize smpc
  smpc.reg.DDR1 = 0; // INPUT
  smpc.reg.DDR2 = 0; // INPUT
  smpc.reg.IOSEL = 0; // SMPC control
  smpc.reg.EXLE = 0; //

  // interrupts
  sh2_vec[SCU_VEC__SMPC] = (u32)(&smpc_int);
  sh2_vec[SCU_VEC__V_BLANK_IN] = (u32)(&v_blank_in_int);

  scu.reg.IST = 0;
  scu.reg.IMS = ~(IMS__SMPC | IMS__V_BLANK_IN);
}

extern "C"
void start(void)
{
  main();
  while (1);
}
