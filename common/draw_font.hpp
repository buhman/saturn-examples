#include <stdint.h>

#include "font.hpp"

namespace draw_font {

struct state {
  uint32_t color_address;
  uint32_t character_address;
  font * _font;
  glyph * _glyphs;
};

uint32_t font_data(void * buf, uint32_t top, state& state);

template <typename T>
uint32_t horizontal_string(state const& s,
                           uint32_t& cmd_ix,
                           const T * string,
                           const uint32_t length,
                           const int32_t x,
                           const int32_t y)
{
  int32_t total_advance = 0;

  for (uint32_t i = 0; i < length; i++) {
    const T c = string[i];
    //assert(c <= s.char_code_offset);
    const T c_offset = c - s._font->char_code_offset;

    glyph_bitmap const& bitmap = s._glyphs[c_offset].bitmap;
    glyph_metrics const& metrics = s._glyphs[c_offset].metrics;

    if (bitmap.pitch != 0) {
      vdp1.vram.cmd[cmd_ix].CTRL = CTRL__JP__JUMP_NEXT | CTRL__COMM__NORMAL_SPRITE;
      vdp1.vram.cmd[cmd_ix].LINK = 0;
      vdp1.vram.cmd[cmd_ix].PMOD = PMOD__COLOR_MODE__COLOR_BANK_256;
      vdp1.vram.cmd[cmd_ix].COLR = s.color_address;
      vdp1.vram.cmd[cmd_ix].SRCA = SRCA(s.character_address + bitmap.offset);
      vdp1.vram.cmd[cmd_ix].SIZE = SIZE__X(bitmap.pitch) | SIZE__Y(bitmap.rows);
      vdp1.vram.cmd[cmd_ix].XA = (total_advance + x + metrics.horiBearingX) >> 6;
      vdp1.vram.cmd[cmd_ix].YA = (y - metrics.horiBearingY) >> 6;
      cmd_ix++;
    }

    total_advance += metrics.horiAdvance;
  }

  return total_advance;
}

template <typename T>
uint32_t single_character_centered(state const& s,
                                   uint32_t cmd_ix,
                                   const T c,
                                   const int32_t x1, // in 26.6 fixed point
                                   const int32_t y1,
                                   const int32_t x2,
                                   const int32_t y2)
{
  //assert(c <= s.char_code_offset);
  const T c_offset = c - s._font->char_code_offset;

  glyph_bitmap const& bitmap = s._glyphs[c_offset].bitmap;
  glyph_metrics const& metrics = s._glyphs[c_offset].metrics;

  constexpr uint16_t magenta = (0x31 << 10) | (0x31 << 0);

  vdp1.vram.cmd[cmd_ix].CTRL = CTRL__JP__JUMP_NEXT | CTRL__COMM__POLYLINE;
  vdp1.vram.cmd[cmd_ix].LINK = 0;
  vdp1.vram.cmd[cmd_ix].PMOD = PMOD__ECD | PMOD__SPD;
  vdp1.vram.cmd[cmd_ix].COLR = COLR__RGB | magenta; // non-palettized (rgb15) color data
  vdp1.vram.cmd[cmd_ix].XA = x1;
  vdp1.vram.cmd[cmd_ix].YA = y1;
  vdp1.vram.cmd[cmd_ix].XB = x2;
  vdp1.vram.cmd[cmd_ix].YB = y1;
  vdp1.vram.cmd[cmd_ix].XC = x2;
  vdp1.vram.cmd[cmd_ix].YC = y2;
  vdp1.vram.cmd[cmd_ix].XD = x1;
  vdp1.vram.cmd[cmd_ix].YD = y2;
  cmd_ix++;

  int32_t x_gap = (((x2 - x1 + 1) << 6) - s._font->max_advance) >> 1;
  int32_t y_gap = (((y2 - y1) << 6) - s._font->height) >> 2;

  if (bitmap.pitch != 0) {
    vdp1.vram.cmd[cmd_ix].CTRL = CTRL__JP__JUMP_NEXT | CTRL__COMM__NORMAL_SPRITE;
    vdp1.vram.cmd[cmd_ix].LINK = 0;
    vdp1.vram.cmd[cmd_ix].PMOD = PMOD__COLOR_MODE__COLOR_BANK_256;
    vdp1.vram.cmd[cmd_ix].COLR = s.color_address;
    vdp1.vram.cmd[cmd_ix].SRCA = SRCA(s.character_address + bitmap.offset);
    vdp1.vram.cmd[cmd_ix].SIZE = SIZE__X(bitmap.pitch) | SIZE__Y(bitmap.rows);
    vdp1.vram.cmd[cmd_ix].XA = ((x1 << 6) + x_gap + metrics.horiBearingX) >> 6;
    vdp1.vram.cmd[cmd_ix].YA = ((y2 << 6) + y_gap - metrics.horiBearingY) >> 6;
    cmd_ix++;
  }

  return cmd_ix;
}

}
