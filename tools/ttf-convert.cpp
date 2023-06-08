#include <sstream>
#include <iostream>

#include <assert.h>
#include <stdint.h>
#include <byteswap.h>
#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "../common/font.hpp"

int32_t
load_outline_char(const FT_Face face,
                  const FT_ULong char_code,
                  glyph * glyph,
                  uint8_t * glyph_bitmaps,
                  const uint32_t bitmap_offset)
{
  FT_Error error;
  FT_UInt glyph_index = FT_Get_Char_Index(face, char_code);

  error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
  if (error) {
    std::cerr << "FT_Load_Glyph " << FT_Error_String(error) << '\n';
    return -1;
  }

  //assert(face->glyph->format == FT_GLYPH_FORMAT_OUTLINE);

  error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
  if (error) {
    std::cerr << "FT_Render_Glyph " << FT_Error_String(error) << '\n';
    return -1;
  }

  uint32_t pitch = (face->glyph->bitmap.pitch + 8 - 1) & -8;
  uint32_t bitmap_size = face->glyph->bitmap.rows * pitch;

  //printf("num_grays %d\n", face->glyph->bitmap.num_grays);

  if (!(face->glyph->bitmap.pitch > 0)) {
    assert(pitch == 0);
    assert(bitmap_size == 0);
    assert(face->glyph->bitmap.width == 0);
    assert(face->glyph->bitmap.rows == 0);
  }

  switch (face->glyph->bitmap.num_grays) {
  case 2:
    {
      for (unsigned int y = 0; y < face->glyph->bitmap.rows; y++) {
	uint8_t * row = &face->glyph->bitmap.buffer[y * face->glyph->bitmap.pitch];
	//uint8_t row_out = 0;
	for (unsigned int x = 0; x < face->glyph->bitmap.width; x++) {
	  int bit;
	  bit = (row[x / 8] >> (7 - (x % 8))) & 1;
	  //fprintf(stderr, bit ? "█" : " ");
	  //row_out |= (bit << x);

	  // FIXME: this is lazy: this bloats the storage format from
	  // 1 bit per pixel to 8 bits per pixel, even though this
	  // expansion could be done at runtime inside vdp1 vram
	  // instead.
	  glyph_bitmaps[bitmap_offset + (y * pitch + x)] = bit ? 31 : 0;
	}
	//fprintf(stderr, "\n");
	//buf[y] = row_out;
      }
    }
    break;
  case 256:
    {
      for (uint32_t y = 0; y < face->glyph->bitmap.rows; y++) {
	for (uint32_t x = 0; x < face->glyph->bitmap.width; x++) {
	  uint8_t i = face->glyph->bitmap.buffer[y * face->glyph->bitmap.pitch + x];
	  glyph_bitmaps[bitmap_offset + (y * pitch + x)] = i >> 3;
	}
      }
    }
    break;
  default:
    assert(-1 == face->glyph->bitmap.num_grays);
  }

  //memcpy(&glyph_bitmaps[bitmap_offset], face->glyph->bitmap.buffer, bitmap_size);

  glyph_bitmap& bitmap = glyph->bitmap;
  bitmap.offset = bswap_32(bitmap_offset);
  bitmap.rows = bswap_32(face->glyph->bitmap.rows);
  bitmap.width = bswap_32(face->glyph->bitmap.width);
  bitmap.pitch = bswap_32(pitch);
  /*
  std::cerr << std::hex << char_code
            << std::dec << ' ' << pitch
            << std::dec << ' ' << face->glyph->bitmap.width
            << '\n';
  */

  assert((pitch % 8) == 0);

  glyph_metrics& metrics = glyph->metrics;
  metrics.width = bswap_32(face->glyph->metrics.width);
  metrics.height = bswap_32(face->glyph->metrics.height);
  metrics.horiBearingX = bswap_32(face->glyph->metrics.horiBearingX);
  metrics.horiBearingY = bswap_32(face->glyph->metrics.horiBearingY);
  metrics.horiAdvance = bswap_32(face->glyph->metrics.horiAdvance);

  return bitmap_size;
}

struct range {
  uint32_t start;
  uint32_t end;
};

int main(int argc, char *argv[])
{
  FT_Library library;
  FT_Face face;
  FT_Error error;

  if (argc != 6) {
    std::cerr << "usage: " << argv[0] << " [start-hex] [end-hex] [pixel-size] [font-file-path] [output-file-path]\n\n";
    std::cerr << "   ex: " << argv[0] << " 3000 30ff 30 ipagp.ttf font.bin\n";
    return -1;
  }

  error = FT_Init_FreeType(&library);
  if (error) {
    std::cerr << "FT_Init_FreeType\n";
    return -1;
  }

  error = FT_New_Face(library, argv[4], 0, &face);
  if (error) {
    std::cerr << "FT_New_Face\n";
    return -1;
  }

  std::stringstream ss3;
  int font_size;
  ss3 << std::dec << argv[3];
  ss3 >> font_size;
  std::cerr << "font_size: " << font_size << '\n';

  error = FT_Set_Pixel_Sizes(face, 0, font_size);
  if (error) {
    std::cerr << "FT_Set_Pixel_Sizes: " << FT_Error_String(error) << error << '\n';
    return -1;
  }

  uint32_t start;
  uint32_t end;

  std::stringstream ss1;
  ss1 << std::hex << argv[1];
  ss1 >> start;
  std::stringstream ss2;
  ss2 << std::hex << argv[2];
  ss2 >> end;

  glyph glyphs[(end - start) + 1];
  uint8_t glyph_bitmaps[1024 * 1024];
  memset(glyph_bitmaps, 0x00, 1024 * 1024);
  uint32_t bitmap_offset = 0, glyph_index = 0;
  int32_t bitmap_size;

  for (uint32_t char_code = start; char_code <= end; char_code++) {
    bitmap_size = load_outline_char(face,
                                    char_code,
                                    &glyphs[glyph_index],
                                    &glyph_bitmaps[0],
                                    bitmap_offset);
    if (bitmap_size < 0) {
      std::cerr << "load_outline_char error\n";
      return -1;
    }

    bitmap_offset += bitmap_size;
    assert(bitmap_offset < (sizeof (glyph_bitmaps)));
    glyph_index++;
  }

  font font;
  font.char_code_offset = bswap_32(start);
  font.glyph_index = bswap_32(glyph_index);
  font.bitmap_offset = bswap_32(bitmap_offset);
  font.height = bswap_32(face->size->metrics.height);
  font.max_advance = bswap_32(face->size->metrics.max_advance);

  FT_Done_FreeType(library);

  std::cerr << "start: 0x" << std::hex << start << '\n';
  std::cerr << "end: 0x" << std::hex << end << '\n';
  std::cerr << "bitmap_offset: 0x" << std::hex << bitmap_offset << '\n';
  std::cerr << "glyph_index: 0x" << std::hex << glyph_index << '\n';

  FILE * out = fopen(argv[5], "w");
  if (out == NULL) {
    perror("fopen(w)");
    return -1;
  }

  fwrite(reinterpret_cast<void*>(&font), (sizeof (font)), 1, out);
  fwrite(reinterpret_cast<void*>(&glyphs[0]), (sizeof (glyph)), glyph_index, out);
  fwrite(reinterpret_cast<void*>(&glyph_bitmaps[0]), (sizeof (uint8_t)), bitmap_offset, out);

  fclose(out);
}
