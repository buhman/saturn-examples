#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <ft2build.h>
#include FT_FREETYPE_H

int
load_bitmap_char(FT_Face face,
		 FT_ULong char_code,
		 uint8_t * buf)
{
  FT_Error error;
  FT_UInt glyph_index = FT_Get_Char_Index(face, char_code);

  error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
  if (error) {
    fprintf(stderr, "FT_Load_Glyph %s\n", FT_Error_String(error));
    return -1;
  }

  printf("horiBearingX: %ld\n", face->glyph->metrics.horiBearingX >> 6);
  printf("horiBearingY: %ld\n", face->glyph->metrics.horiBearingY >> 6);
  printf("horiAdvance: %ld\n", face->glyph->metrics.horiAdvance >> 6);
  printf("width: %ld\n", face->glyph->metrics.width >> 6);
  printf("height: %ld\n", face->glyph->metrics.height >> 6);

  assert(face->glyph->format == FT_GLYPH_FORMAT_BITMAP);
  assert(face->glyph->bitmap.num_grays == 2);

  for (int y = 0; y < (int)face->glyph->bitmap.rows; y++) {
    uint8_t * row = &face->glyph->bitmap.buffer[y * face->glyph->bitmap.pitch];
    for (int x = 0; x < (int)face->glyph->bitmap.width; x += 1) {
      const int bit = (row[x / 8] >> (7 - (x % 8))) & 1;
      //std::cerr << (bit ? "█" : " ");
      buf[y * face->glyph->bitmap.width + x] = bit;
    }
    //std::cerr << "|\n";
  }

  return face->glyph->bitmap.rows * face->glyph->bitmap.width;
}

int load_font(FT_Library * library, FT_Face * face, const char * font_file_path)
{
  FT_Error error;

  error = FT_Init_FreeType(library);
  if (error) {
    fprintf(stderr, "FT_Init_FreeType\n");
    return -1;
  }

  error = FT_New_Face(*library, font_file_path, 0, face);
  if (error) {
    fprintf(stderr, "FT_New_Face\n");
    return -1;
  }

  error = FT_Select_Size(*face, 0);
  if (error) {
    fprintf(stderr, "FT_Select_Size: %d: %s\n", error, FT_Error_String(error));
    return -1;
  }

  return 0;
}

void usage(const char * argv_0)
{
  printf("%s [start-hex] [end-hex] [font-width] [font-height] [font-file-path] [output-file-path]\n", argv_0);
}

void pack_4bit(const uint8_t * src, int width, int height, int size, uint8_t * dst)
{
  int offset = 0;
  while (offset < size) {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x += 2) {
        int px0 = src[offset + y * width + x + 0];
        int px1 = src[offset + y * width + x + 1];
        dst[(offset / 2) + y * (width / 2) + (x / 2)] = (px0 << 4) | (px1 << 0);
      }
    }
    offset += width * height;
  }
}

int main(int argc, const char * argv[])
{
  if (argc != 7) {
    usage(argv[0]);
    return -1;
  }

  char * endptr;

  int start_hex = strtol(argv[1], &endptr, 16);
  assert(*endptr == 0);
  int end_hex = strtol(argv[2], &endptr, 16);
  assert(*endptr == 0);

  int font_width = strtol(argv[3], &endptr, 10);
  assert(*endptr == 0);
  int font_height = strtol(argv[4], &endptr, 10);
  assert(*endptr == 0);

  const char * font_file_path = argv[5];
  const char * output_file_path = argv[6];

  printf("start_hex %x\n", start_hex);
  printf("end_hex %x\n", start_hex);
  printf("font_width %d\n", font_width);
  printf("font_height %d\n", font_height);
  printf("font_file_path %s\n", font_file_path);
  printf("output_file_path %s\n", output_file_path);

  FT_Library library;
  FT_Face face;
  int res;
  res = load_font(&library, &face, font_file_path);
  if (res < 0)
    return -1;

  int texture_buf_size = font_width * font_height * ((end_hex - start_hex) + 1);
  uint8_t * texture = (uint8_t *)malloc(texture_buf_size);

  int offset = 0;

  for (int char_code = start_hex; char_code <= end_hex; char_code++) {
    assert(offset < texture_buf_size);

    res = load_bitmap_char(face,
                           char_code,
                           &texture[offset]);
    if (res < 0)
      return - 1;

    assert(res == font_width * font_height);

    offset += res;
  }

  uint8_t * pack = (uint8_t *)malloc(texture_buf_size / 2);
  pack_4bit(texture, font_width, font_height, offset, pack);

  FILE * out = fopen(output_file_path, "w");
  if (out == NULL) {
    perror("fopen(w)");
    return -1;
  }
  //fwrite((void *)texture, texture_buf_size, 1, out);
  fwrite((void *)pack, texture_buf_size / 2, 1, out);
  fclose(out);
}
