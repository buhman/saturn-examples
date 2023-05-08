#include <stdint.h>
#include "vdp2.h"
#include "vdp1.h"

extern void * _ipafont_data_start __asm("_binary_res_ipapgothic_font_bin_start");

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

/*
uint32_t character_pattern_table(const uint32_t top)
{
  // Unlike vdp2 cell format, vdp1 sprites appear to be much more dimensionally
  // flexible. The data is interpreted as a row-major packed array, where the
  // row/horizontal stride is equal to the sprite width (as configured in the
  // draw command). This is identical to how the input palette index data is
  // structured, so there is no transformation to do here, only a plain memory
  // copy.

  // Multiply `buf_size` by one because this converts (indexed color) 8 bit pixels
  // to 8 bit pixels. Round up to the nearest 0x20 (for an 8000 pixel/8000 byte
  // image, this rounding is a no-op).

  const uint32_t table_size = ((buf_size * 1) + 0x20 - 1) & (-0x20);
  const uint32_t table_address = top - table_size;
  uint16_t * table = &vdp1.vram.u16[(table_address / 2)];

  // `table_size` is in bytes; divide by two to get uint16_t indicies.
  uint32_t buf_ix = 0;
  for (uint32_t table_ix = 0; table_ix < (table_size / 2); table_ix++) {
    uint32_t tmp = buf[buf_ix];

    table[table_ix] = (((tmp >> 8) & 0xff) << 8)
                    | (((tmp >> 0) & 0xff) << 0);

    buf_ix += 1;
  }

  return table_address;
}
*/

// metrics are 26.6 fixed point
struct glyph_metrics {
  int32_t width;
  int32_t height;
  int32_t horiBearingX;
  int32_t horiBearingY;
  int32_t horiAdvance;
} __attribute__ ((packed));

static_assert((sizeof (glyph_metrics)) == ((sizeof (int32_t)) * 5));

struct glyph_bitmap {
  uint32_t offset;
  uint32_t rows;
  uint32_t width;
  uint32_t pitch;
} __attribute__ ((packed));

static_assert((sizeof (glyph_bitmap)) == ((sizeof (uint32_t)) * 4));

struct glyph {
  glyph_bitmap bitmap;
  glyph_metrics metrics;
} __attribute__ ((packed));

static_assert((sizeof (glyph)) == ((sizeof (glyph_bitmap)) + (sizeof (glyph_metrics))));

struct font {
  uint32_t glyph_index;
  uint32_t bitmap_offset;
  int32_t height;
  int32_t max_advance;
} __attribute__ ((packed));

static_assert((sizeof (font)) == ((sizeof (uint32_t)) * 4));

template <typename T>
void copy(T * dst, const T * src, int32_t n) noexcept
{
  while (n > 0) {
    *dst++ = *src++;
    n -= (sizeof (T));
  }
}

uint32_t pixel_data(const uint32_t top, const uint8_t * glyph_bitmaps, const uint32_t bitmap_offset)
{
  const uint32_t * buf = reinterpret_cast<const uint32_t *>(&glyph_bitmaps[0]);

  const uint32_t table_size = (bitmap_offset + 0x20 - 1) & (-0x20);
  const uint32_t table_address = top - table_size;

  uint32_t * table = &vdp1.vram.u32[(table_address / 4)];

  copy<uint32_t>(table, buf, bitmap_offset);

  return table_address;
}

uint32_t draw_utf16_string(const uint32_t color_address,
			   const uint32_t character_address,
			   const glyph * glyphs,
			   uint32_t cmd_ix,
			   const char16_t * string,
			   const uint32_t length)
{
  int32_t x = 8 << 6;
  int32_t y = 100 << 6;

  for (uint32_t i = 0; i < length; i++) {
    const char16_t c = string[i];
    const uint16_t c_offset = c - 0x3000;

    const glyph_bitmap& bitmap = glyphs[c_offset].bitmap;
    const glyph_metrics& metrics = glyphs[c_offset].metrics;

    vdp1.vram.cmd[cmd_ix].CTRL = CTRL__JP__JUMP_NEXT | CTRL__COMM__NORMAL_SPRITE;
    vdp1.vram.cmd[cmd_ix].LINK = 0;
    vdp1.vram.cmd[cmd_ix].PMOD = PMOD__ECD | PMOD__COLOR_MODE__COLOR_BANK_256;
    vdp1.vram.cmd[cmd_ix].COLR = color_address; // non-palettized (rgb15) color data
    vdp1.vram.cmd[cmd_ix].SRCA = SRCA(character_address + bitmap.offset);
    vdp1.vram.cmd[cmd_ix].SIZE = SIZE__X(bitmap.pitch) | SIZE__Y(bitmap.rows);
    vdp1.vram.cmd[cmd_ix].XA = (x + metrics.horiBearingX) >> 6;
    vdp1.vram.cmd[cmd_ix].YA = (y - metrics.horiBearingY) >> 6;

    x += metrics.horiAdvance;

    cmd_ix++;
  }

  return cmd_ix;
}

static const char16_t _string[] = u"「ブルースパン」";

void main()
{
  uint32_t color_address, character_address;
  uint32_t top = (sizeof (union vdp1_vram));
  constexpr uint32_t colors = 256;
  constexpr uint32_t color_bank = 0; // completely random and arbitrary value

  uint8_t * data = reinterpret_cast<uint8_t*>(&_ipafont_data_start);

  const font * font = reinterpret_cast<struct font*>(&data[0]);
  const glyph * glyphs = reinterpret_cast<struct glyph*>(&data[(sizeof (struct font))]);
  // there are three 32-bit fields before the start of `glyphs`
  const uint8_t * glyph_bitmaps = &data[(sizeof (struct font)) + ((sizeof (struct glyph)) * font->glyph_index)];

  top = character_address = pixel_data(top, glyph_bitmaps, font->bitmap_offset);

  vdp2_color_palette(colors, color_bank);
  // For color bank color, COLR is concatenated bitwise with pixel data. See
  // Figure 6.17 in the VDP1 manual.
  color_address = color_bank << 8;

  // DISP: Please make sure to change this bit from 0 to 1 during V blank.
  vdp2.reg.TVMD = ( TVMD__DISP | TVMD__LSMD__NON_INTERLACE
                  | TVMD__VRESO__240 | TVMD__HRESO__NORMAL_320);

  // disable all VDP2 backgrounds (e.g: the Sega bios logo)
  vdp2.reg.BGON = 0;

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

  uint32_t cmd_ix = 2;
  cmd_ix = draw_utf16_string(color_address,
			     character_address,
			     glyphs,
			     cmd_ix,
			     _string,
			     ((sizeof (_string)) / (sizeof (_string[0]))) - 1);

  vdp1.vram.cmd[cmd_ix].CTRL = CTRL__END;
  cmd_ix++;

  // start drawing (execute the command list) on every frame
  vdp1.reg.PTMR = PTMR__PTM__FRAME_CHANGE;
}

extern "C"
void start(void)
{
  main();
  while (1) {}
}
