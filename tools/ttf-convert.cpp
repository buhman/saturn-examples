#include <sstream>
#include <iostream>

#include <assert.h>
#include <stdint.h>
#include <byteswap.h>
#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H

/*
int load_bitmap_char(FT_Face face, FT_ULong char_code, uint8_t * buf)
{
  FT_Error error;
  FT_UInt glyph_index = FT_Get_Char_Index(face, char_code);

  error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
  if (error) {
    printf("FT_Load_Glyph %s\n", FT_Error_String(error));
    return 0;
  }

  assert(face->glyph->format == FT_GLYPH_FORMAT_BITMAP);
  printf("num_grays %d\n", face->glyph->bitmap.num_grays);
  printf("pitch %d\n", face->glyph->bitmap.pitch);
  printf("width %d\n", face->glyph->bitmap.width);
  assert(face->glyph->bitmap.width == 16);
  printf("char_code %lx rows %d\n", char_code, face->glyph->bitmap.rows);
  assert(face->glyph->bitmap.rows == 8 || (face->glyph->bitmap.rows % 8) == 0);

  for (int y = 0; y < (int)face->glyph->bitmap.rows; y++) {
    uint8_t * row = &face->glyph->bitmap.buffer[y * face->glyph->bitmap.pitch];
    uint8_t row_out = 0;
    for (int x = 0; x < face->glyph->bitmap.width; x++) {
      int bit;
      if (x < (int)face->glyph->bitmap.width) {
        bit = (row[x / 8] >> (7 - (x % 8))) & 1;
      } else {
        bit = 0;
      }
      fprintf(stderr, bit ? "█" : " ");
      row_out |= (bit << x);
    }
    fprintf(stderr, "\n");
    //buf[y] = row_out;
  }

  return face->glyph->bitmap.rows * face->glyph->bitmap.pitch;
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
  uint32_t char_code_offset;
  uint32_t glyph_index;
  uint32_t bitmap_offset;
  int32_t height;
  int32_t max_advance;
} __attribute__ ((packed));

static_assert((sizeof (font)) == ((sizeof (uint32_t)) * 5));

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

  assert(face->glyph->format == FT_GLYPH_FORMAT_OUTLINE);

  error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
  if (error) {
    std::cerr << "FT_Render_Glyph " << FT_Error_String(error) << '\n';
    return -1;
  }

  if (!(face->glyph->bitmap.pitch > 0)) {
    std::cerr << std::hex << char_code << " : pitch == 0\n";
    return 0;
  }

  uint32_t pitch = (face->glyph->bitmap.pitch + 8 - 1) & -8;
  uint32_t bitmap_size = face->glyph->bitmap.rows * pitch;

  for (uint32_t y = 0; y < face->glyph->bitmap.rows; y++) {
    for (uint32_t x = 0; x < face->glyph->bitmap.width; x++) {
      uint8_t i = face->glyph->bitmap.buffer[y * face->glyph->bitmap.pitch + x];
      glyph_bitmaps[bitmap_offset + (y * pitch + x)] = i >> 3;
    }
  }

  //memcpy(&glyph_bitmaps[bitmap_offset], face->glyph->bitmap.buffer, bitmap_size);

  glyph_bitmap& bitmap = glyph->bitmap;
  bitmap.offset = bswap_32(bitmap_offset);
  bitmap.rows = bswap_32(face->glyph->bitmap.rows);
  bitmap.width = bswap_32(face->glyph->bitmap.width);
  bitmap.pitch = bswap_32(pitch);
  //printf("%lx: %d %d\n", char_code, pitch, face->glyph->bitmap.width);
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

  if (argc < 5) {
    std::cerr << "usage: " << argv[0] << " [start-hex] [end-hex] [font-file-path] [output-file-path]\n\n";
    std::cerr << "   ex: " << argv[0] << " 3000 30ff ipagp.ttf font.bin\n";
    return -1;
  }

  error = FT_Init_FreeType(&library);
  if (error) {
    std::cerr << "FT_Init_FreeType\n";
    return -1;
  }

  error = FT_New_Face(library, argv[3], 0, &face);
  if (error) {
    std::cerr << "FT_New_Face\n";
    return -1;
  }

  /*
  error = FT_Select_Size(face, 0);
  if (error) {
    fprintf(stderr, "FT_Select_Size: %s %d\n", FT_Error_String(error), error);
    return -1;
  }
  */

  error = FT_Set_Pixel_Sizes (face, 0, 30);
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

  //uint32_t start = 0x20;
  //uint32_t end = 0x7f;

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

  FILE * out = fopen(argv[4], "w");

  fwrite(reinterpret_cast<void*>(&font), (sizeof (font)), 1, out);
  fwrite(reinterpret_cast<void*>(&glyphs[0]), (sizeof (glyph)), glyph_index, out);
  fwrite(reinterpret_cast<void*>(&glyph_bitmaps[0]), (sizeof (uint8_t)), bitmap_offset, out);

  fclose(out);
}
