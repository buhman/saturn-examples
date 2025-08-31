#include <stdint.h>

#include "vdp2.h"
#include "../common/vdp2_func.hpp"

#include "aseprite/rustboro/palette.bin.h"
#include "aseprite/rustboro/character_pattern__tileset_0.bin.h"
#include "aseprite/rustboro/character_pattern__tileset_1.bin.h"
#include "aseprite/rustboro/pattern_name_table__layer_0.bin.h"
#include "aseprite/rustboro/pattern_name_table__layer_1.bin.h"

void palette_data()
{
  const uint16_t * buf = reinterpret_cast<uint16_t *>(&_binary_aseprite_rustboro_palette_bin_start);
  const uint32_t buf_size = reinterpret_cast<uint32_t>(&_binary_aseprite_rustboro_palette_bin_size);

  for (uint32_t i = 0; i < buf_size / 2; i++) {
    vdp2.cram.u16[i] = buf[i];
  }
}

typedef struct buf_size {
  uint32_t const * const buf;
  const uint32_t size;
} buf_size_t;

const buf_size_t character_patterns[] = {
  {
    .buf = (uint32_t *)(&_binary_aseprite_rustboro_character_pattern__tileset_0_bin_start),
    .size = (uint32_t)(&_binary_aseprite_rustboro_character_pattern__tileset_0_bin_size),
  },
  {
    .buf = (uint32_t *)(&_binary_aseprite_rustboro_character_pattern__tileset_1_bin_start),
    .size = (uint32_t)(&_binary_aseprite_rustboro_character_pattern__tileset_1_bin_size),
  },
};

uint32_t character_pattern_data(const buf_size_t * buf_size, const uint32_t top)
{
  const uint32_t base_address = top - buf_size->size; // in bytes

  for (uint32_t i = 0; i < (buf_size->size / 4); i++) {
    vdp2.vram.u32[(base_address / 4) + i] = buf_size->buf[i];
  }

  return base_address;
}

const buf_size_t pattern_name_tables[] = {
  {
    .buf = (uint32_t *)(&_binary_aseprite_rustboro_pattern_name_table__layer_0_bin_start),
    .size = (uint32_t)(&_binary_aseprite_rustboro_pattern_name_table__layer_0_bin_size),
  },
  {
    .buf = (uint32_t *)(&_binary_aseprite_rustboro_pattern_name_table__layer_1_bin_start),
    .size = (uint32_t)(&_binary_aseprite_rustboro_pattern_name_table__layer_1_bin_size),
  },
};

void pattern_name_table_data(const buf_size_t * buf_size,
                             const uint32_t vram_offset,
                             const uint32_t character_offset)
{
  for (uint32_t i = 0; i < buf_size->size / 4; i++) {
    uint32_t data = buf_size->buf[i];
    uint32_t character_number = (data & 0x7fff) * 8 + character_offset;
    uint32_t flags = data & 0xffff0000;
    vdp2.vram.u32[(vram_offset / 4) + i] = flags | character_number;
  }
}

void main()
{
  v_blank_in();

  // DISP: Please make sure to change this bit from 0 to 1 during V blank.
  vdp2.reg.TVMD = ( TVMD__DISP | TVMD__LSMD__NON_INTERLACE
                  | TVMD__VRESO__240 | TVMD__HRESO__NORMAL_320);


  /* set the color mode to 5bits per channel, 1024 colors */
  vdp2.reg.RAMCTL = RAMCTL__CRMD__RGB_5BIT_1024;

  vdp2.reg.PRINA = PRINA__N0PRIN(1) | PRINA__N1PRIN(2);

  vdp2.reg.VRSIZE = 0;

  /* enable display of NBG0 */
  vdp2.reg.BGON = BGON__N0ON | BGON__N0TPON
                | BGON__N1ON;

  /* set character format for NBG0 to palettized 16 color
     set enable "cell format" for NBG0
     set character size for NBG0 to 1x1 cell */
  vdp2.reg.CHCTLA = CHCTLA__N0CHCN__256_COLOR
                  | CHCTLA__N0BMEN__CELL_FORMAT
                  | CHCTLA__N0CHSZ__2x2_CELL
                  | CHCTLA__N1CHCN__256_COLOR
                  | CHCTLA__N1BMEN__CELL_FORMAT
                  | CHCTLA__N1CHSZ__2x2_CELL;

  /* plane size */
  vdp2.reg.PLSZ = PLSZ__N0PLSZ__2x2 | PLSZ__N1PLSZ__2x2;

  /* map plane offset
     1-word: value of bit 6-0 * 0x2000
     2-word: value of bit 5-0 * 0x4000
  */
  constexpr int plane_a = 0;
  constexpr int plane_a_offset = plane_a * 0x1000;

  constexpr int plane_b = 4;
  constexpr int plane_b_offset = plane_b * 0x1000;

// Enable VRAM bank partitioning
  vdp2.reg.RAMCTL = RAMCTL__VRAMD | RAMCTL__VRBMD;

  vdp2.reg.MPOFN = MPOFN__N1MP(0) | MPOFN__N0MP(0); // bits 8~6

  vdp2.reg.MPABN0 = MPABN0__N0MPB(plane_a) | MPABN0__N0MPA(plane_a); // bits 5~0
  vdp2.reg.MPCDN0 = MPCDN0__N0MPD(plane_a) | MPCDN0__N0MPC(plane_a); // bits 5~0
  vdp2.reg.PNCN0 = PNCN0__N0PNB__2WORD;

  vdp2.reg.MPABN1 = MPABN1__N1MPB(plane_b) | MPABN1__N1MPA(plane_b); // bits 5~0
  vdp2.reg.MPCDN1 = MPCDN1__N1MPD(plane_b) | MPCDN1__N1MPC(plane_b); // bits 5~0
  vdp2.reg.PNCN1 = PNCN1__N1PNB__2WORD;

  // cpu access
  vdp2.reg.CYCA0 = 0xeeeeeeee;
  vdp2.reg.CYCA1 = 0xeeeeeeee;
  vdp2.reg.CYCB0 = 0xeeeeeeee;
  vdp2.reg.CYCB1 = 0xeeeeeeee;

  uint32_t top = (sizeof (union vdp2_vram));
  palette_data();

  top = character_pattern_data(&character_patterns[0], top);
  uint32_t pattern_base_0 = top / 32;

  top = character_pattern_data(&character_patterns[1], top);
  uint32_t pattern_base_1 = top / 32;

  pattern_name_table_data(&pattern_name_tables[0],
                          plane_a_offset,
                          pattern_base_0);

  pattern_name_table_data(&pattern_name_tables[1],
                          plane_b_offset,
                          pattern_base_1);

  int sx = 0;
  int sy = 0;
  int dsx = 1;
  int dsy = 1;

  vdp2.reg.CYCA0 = 0x01ffffff;
  vdp2.reg.CYCA1 = 0x01ffffff;
  vdp2.reg.CYCB0 = 0x45ffffff;
  vdp2.reg.CYCB1 = 0x45ffffff;

  while (true) {
    vdp2.reg.SCXIN0 = sx >> 2;
    vdp2.reg.SCXDN0 = (sx & 0b11) << 14;
    vdp2.reg.SCYIN0 = sy >> 1;
    vdp2.reg.SCYDN0 = (sy & 1) << 15;

    vdp2.reg.SCXIN1 = sx >> 2;
    vdp2.reg.SCXDN1 = (sx & 0b11) << 14;
    vdp2.reg.SCYIN1 = sy >> 1;
    vdp2.reg.SCYDN1 = (sy & 1) << 15;

    if ((sx >> 2) >= 320 + 10)
      dsx = -1;
    if ((sx >> 2) <= 0 - 10)
      dsx = +1;
    if ((sy >> 1) >= 720 + 10)
      dsy = -1;
    if ((sy >> 1) <= 0 - 10)
      dsy = +1;

    sx += dsx;
    sy += dsy;

    while ((vdp2.reg.TVSTAT & TVSTAT__VBLANK) != 0);
    while ((vdp2.reg.TVSTAT & TVSTAT__VBLANK) == 0);
  }
}
