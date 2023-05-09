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
