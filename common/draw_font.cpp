#include "vdp1.h"

#include "font.hpp"
#include "copy.hpp"
#include "draw_font.hpp"

uint32_t pixel_data(const uint32_t top, const uint8_t * glyph_bitmaps, const uint32_t size)
{
  const uint32_t table_size = (size + 0x20 - 1) & (-0x20);
  const uint32_t table_address = top - table_size;

  uint32_t * table = &vdp1.vram.u32[(table_address / 4)];

  const uint32_t * buf = reinterpret_cast<const uint32_t *>(&glyph_bitmaps[0]);

  copy<uint32_t>(table, buf, size);

  return table_address;
}

namespace draw_font {

uint32_t font_data(void * buf, uint32_t top, state& state)
{
  uint8_t * data = reinterpret_cast<uint8_t*>(buf);

  font * font = reinterpret_cast<struct font*>(&data[0]);
  glyph * glyphs = reinterpret_cast<struct glyph*>(&data[(sizeof (struct font))]);
  // there are three 32-bit fields before the start of `glyphs`
  const uint8_t * glyph_bitmaps = &data[(sizeof (struct font)) + ((sizeof (struct glyph)) * font->glyph_index)];

  top = pixel_data(top, glyph_bitmaps, font->bitmap_offset);

  state.character_address = top;
  state._font = font;
  state._glyphs = glyphs;

  return top;
}

}
