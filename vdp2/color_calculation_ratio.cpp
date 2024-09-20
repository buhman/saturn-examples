#include <stdint.h>
#include "vdp2.h"
#include "vdp1.h"

#include "../common/vdp2_func.hpp"
#include "../common/copy.hpp"

extern void * _mai_data_pal_start __asm("_binary_res_mai_data_pal_start");
extern void * _mai_data_pal_size __asm("_binary_res_mai_data_pal_size");

extern void * _mai00_data_start __asm("_binary_res_mai00_data_start");
extern void * _mai00_data_size __asm("_binary_res_mai00_data_size");

extern void * _haohmaru_data_pal_start __asm("_binary_res_haohmaru_data_pal_start");
extern void * _haohmaru_data_pal_size __asm("_binary_res_haohmaru_data_pal_size");

extern void * _haohmaru_data_start __asm("_binary_res_haohmaru_data_start");
extern void * _haohmaru_data_size __asm("_binary_res_haohmaru_data_size");

extern void * _forest_pattern_start __asm("_binary_res_forest_pattern_start");
extern void * _forest_pattern_size __asm("_binary_res_forest_pattern_size");

extern void * _forest_tile_start __asm("_binary_res_forest_tile_start");
extern void * _forest_tile_size __asm("_binary_res_forest_tile_size");

extern void * _forest_data_pal_start __asm("_binary_res_forest_data_pal_start");
extern void * _forest_data_pal_size __asm("_binary_res_forest_data_pal_size");

inline constexpr uint16_t rgb15(const uint8_t * rgb24)
{
  return ((rgb24[2] >> 3) << 10) // blue
       | ((rgb24[1] >> 3) << 5)  // green
       | ((rgb24[0] >> 3) << 0); // red
}

void vdp2_color_palette(const uint32_t color_index_offset,
			const uint8_t * buf,
			const uint32_t buf_size)
{
  uint16_t * table = &vdp2.cram.u16[color_index_offset];

  uint32_t buf_ix = 0;
  for (uint32_t i = 0; i < (buf_size / 3); i++) {
    table[i] = rgb15(&buf[buf_ix]);
    buf_ix += 3;
  }
}

void vdp2_color_palette()
{
  { /* mai palette */
    const uint32_t buf_size = reinterpret_cast<uint32_t>(&_mai_data_pal_size);
    const uint8_t * buf = reinterpret_cast<uint8_t*>(&_mai_data_pal_start);

    vdp2_color_palette(0, buf, buf_size);
  }

  { /* forest palette */
    const uint32_t buf_size = reinterpret_cast<uint32_t>(&_forest_data_pal_size);
    const uint8_t * buf = reinterpret_cast<uint8_t*>(&_forest_data_pal_start);

    vdp2_color_palette(16, buf, buf_size);
  }

  { /* haohmaru palette */
    const uint32_t buf_size = reinterpret_cast<uint32_t>(&_haohmaru_data_pal_size);
    const uint8_t * buf = reinterpret_cast<uint8_t*>(&_haohmaru_data_pal_start);

    vdp2_color_palette(32, buf, buf_size);
  }
}

uint32_t character_pattern_table(const uint32_t top, const uint32_t * buf, const uint32_t buf_size)
{
  // Unlike vdp2 cell format, vdp1 sprites appear to be much more dimensionally
  // flexible. The data is interpreted as a row-major packed array, where the
  // row/horizontal stride is equal to the sprite width (as configured in the
  // draw command). This is identical to how the input palette index data is
  // structured, so there is no transformation to do here, only a plain memory
  // copy.

  // Divide `buf_size` by two because this converts (indexed color) 8 bit pixels
  // to 4 bit pixels. Round up to the nearest 0x20 (for an 8000 pixel/8000 byte
  // image, this rounding is a no-op).
  const uint32_t table_size = ((buf_size / 2) + 0x20 - 1) & (-0x20);
  const uint32_t table_address = top - table_size;
  uint16_t * table = &vdp1.vram.u16[(table_address / 2)];

  // `table_size` is in bytes; divide by two to get uint16_t indicies.
  uint32_t buf_ix = 0;
  for (uint32_t table_ix = 0; table_ix < (table_size / 2); table_ix++) {
    uint32_t tmp = buf[buf_ix];

    table[table_ix] = (((tmp >> 24) & 0xf) << 12)
                    | (((tmp >> 16) & 0xf) << 8 )
                    | (((tmp >> 8 ) & 0xf) << 4 )
                    | (((tmp >> 0 ) & 0xf) << 0 );

    buf_ix += 1;
  }

  return table_address;
}

uint32_t forest_cell_data(uint32_t top)
{
  const uint32_t buf_size = reinterpret_cast<uint32_t>(&_forest_tile_size);
  const uint32_t * buf = reinterpret_cast<uint32_t*>(&_forest_tile_start);

  // round to nearest multiple of 32
  const uint32_t table_size = ((buf_size) + 0x20 - 1) & (-0x20);
  const uint32_t base_address = top - table_size; // in bytes

  copy<uint32_t>(&vdp2.vram.u32[base_address / 4], buf, buf_size);

  return base_address;
}

void forest_init()
{
  /* enable display of NBG0 */
  vdp2.reg.BGON = BGON__N0ON | BGON__N0TPON;

  /* set character format for NBG0 to palettized 16 color
     set enable "cell format" for NBG0
     set character size for NBG0 to 1x1 cell */
  vdp2.reg.CHCTLA = CHCTLA__N0CHCN__16_COLOR
                  | CHCTLA__N0BMEN__CELL_FORMAT
                  | CHCTLA__N0CHSZ__1x1_CELL;

  /* plane size */
  vdp2.reg.PLSZ = PLSZ__N0PLSZ__1x1;

  /* map plane offset
     1-word: value of bit 6-0 * 0x2000
     2-word: value of bit 5-0 * 0x4000
  */
  constexpr int plane_a = 0;
  constexpr int plane_a_offset = plane_a * 0x4000;

  vdp2.reg.MPOFN = MPOFN__N0MP(0); // bits 8~6
  vdp2.reg.MPABN0 = MPABN0__N0MPB(plane_a) | MPABN0__N0MPA(plane_a); // bits 5~0
  vdp2.reg.MPCDN0 = MPCDN0__N0MPD(plane_a) | MPCDN0__N0MPC(plane_a); // bits 5~0

  uint32_t top = (sizeof (union vdp2_vram));

  uint32_t cell_top = top = forest_cell_data(0x080000);
  uint32_t pattern_name = cell_top / 32;

  /* use 2-word (32-bit) pattern names */
  vdp2.reg.PNCN0 = PNCN0__N0PNB__2WORD;

  const uint32_t buf_size = reinterpret_cast<uint32_t>(&_forest_pattern_size);
  const uint16_t * buf = reinterpret_cast<uint16_t*>(&_forest_pattern_start);

  uint32_t * pattern = &vdp2.vram.u32[(plane_a_offset / 4)];
  uint32_t x = 0;
  uint32_t y = 0;
  for (uint32_t i = 0; i < (buf_size / 2); i++) {
    pattern[y * 64 + x] = 1 << 16 | (buf[i] + (cell_top / 32));
    x++;
    if (x >= 40) {
      x = 0;
      y++;
    }
  }
}

void main()
{
  // Sega Saturn has 4 Mbit VRAM
  vdp2.reg.VRSIZE = 0;
  // Disable VRAM bank partitioning during CPU access
  vdp2.reg.RAMCTL = 0;

  // Enable CPU access to VDP2 VRAM at all cycles
  vdp2.reg.CYCA0 = 0xeeee'eeee;
  vdp2.reg.CYCA1 = 0xeeee'eeee; // A1 is irrelevant because bank partitioning is not enabled yet
  vdp2.reg.CYCB0 = 0xeeee'eeee;
  vdp2.reg.CYCB1 = 0xeeee'eeee; // B1 is irrelevant because bank partitioning is not enabled yet

  // initialize VDP2 VRAM
  vdp2_color_palette();
  forest_init();

  // NBG0 Pattern Name Data read at T0 from bank A0      (0x000000 - 0x01ffff)
  // NBG0 Character Pattern Data read at T0 from bank B1 (0x060000 - 0x07ffff)
  // all other VDP2 VRAM accesses disabled
  //
  // This is because:
  // - N0MPA / N0MPB / N0MPC / N0MPD are at addresses 0x000000 through 0x002000 (bank A0)
  // - forest_cell_data ("Character Pattern Data" for NBG0) is at addresses 0x076a00 through 0x07ffff (bank B1)
  vdp2.reg.CYCA0 = 0x0fff'ffff;
  vdp2.reg.CYCA1 = 0xffff'ffff;
  vdp2.reg.CYCB0 = 0xffff'ffff;
  vdp2.reg.CYCB1 = 0x4fff'ffff;

  // Enable VRAM bank partitioning
  vdp2.reg.RAMCTL = RAMCTL__VRAMD | RAMCTL__VRBMD;

  uint32_t mai_character_address;
  uint32_t haohmaru_character_address;
  uint32_t top = (sizeof (union vdp1_vram));

  { /* mai */
    const uint32_t buf_size = reinterpret_cast<uint32_t>(&_mai00_data_size);
    const uint32_t * buf = reinterpret_cast<uint32_t*>(&_mai00_data_start);
    top = mai_character_address = character_pattern_table(top, buf, buf_size);
  }

  { /* haohmaru */
    const uint32_t buf_size = reinterpret_cast<uint32_t>(&_haohmaru_data_size);
    const uint32_t * buf = reinterpret_cast<uint32_t*>(&_haohmaru_data_start);
    top = haohmaru_character_address = character_pattern_table(top, buf, buf_size);
  }

  // DISP: Please make sure to change this bit from 0 to 1 during V blank.
  vdp2.reg.TVMD = ( TVMD__DISP | TVMD__LSMD__NON_INTERLACE
                  | TVMD__VRESO__240 | TVMD__HRESO__NORMAL_320);

  // SPCTL__SPCCCS__EQUAL: only perform color calculation on Sprite (pixels)
  // that have a priority number exactly equal ot SPCCN
  //
  // Sprite Data is Type 0
  vdp2.reg.SPCTL = SPCTL__SPCCCS__EQUAL
		 | SPCTL__SPCCN(1)
		 | SPCTL__SPTYPE(0);

  // Enable Sprite Color Calculation
  vdp2.reg.CCCTL = CCCTL__SPCCEN;

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
  vdp2.reg.PRISA = PRISA__S0PRIN(2) // Sprite register 0 PRIority Number
                 | PRISA__S1PRIN(1);
  vdp2.reg.PRINA = PRINA__N0PRIN(1);

  /* TVM settings must be performed from the second H-blank IN interrupt after the
  V-blank IN interrupt to the H-blank IN interrupt immediately after the V-blank
  OUT interrupt. */
  // "normal" display resolution, 16 bits per pixel, 512x256 framebuffer
  vdp1.reg.TVMR = TVMR__TVM__NORMAL;

  // swap framebuffers every 1 cycle; non-interlace
  vdp1.reg.FBCR = 0;

  // during a framebuffer erase cycle, write the color "transparent" to each pixel
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
  vdp1.vram.cmd[2].PMOD = PMOD__ECD | PMOD__COLOR_MODE__COLOR_BANK_16;
  vdp1.vram.cmd[2].COLR = COLR__COLOR_BANK__TYPE0__PR(0)
                        | 0;
  vdp1.vram.cmd[2].SRCA = mai_character_address >> 3;
  vdp1.vram.cmd[2].SIZE = SIZE__X(72) | SIZE__Y(100);
  vdp1.vram.cmd[2].XA = 100;
  vdp1.vram.cmd[2].YA = 100;

  vdp1.vram.cmd[3].CTRL = CTRL__JP__JUMP_NEXT | CTRL__COMM__NORMAL_SPRITE;
  vdp1.vram.cmd[3].LINK = 0;
  // The "end code" is 0xf, which is being used in the mai sprite palette. If
  // both transparency and end codes are enabled, it seems there are only 14
  // usable colors in the 4-bit color mode.
  vdp1.vram.cmd[3].PMOD = PMOD__ECD | PMOD__COLOR_MODE__COLOR_BANK_16;
  vdp1.vram.cmd[3].COLR = COLR__COLOR_BANK__TYPE0__PR(1)
                        | 32;
  vdp1.vram.cmd[3].SRCA = haohmaru_character_address >> 3;
  vdp1.vram.cmd[3].SIZE = SIZE__X(104) | SIZE__Y(132);
  vdp1.vram.cmd[3].XA = 120;
  vdp1.vram.cmd[3].YA = 110;

  vdp1.vram.cmd[4].CTRL = CTRL__END;

  // start drawing (execute the command list) on every frame
  vdp1.reg.PTMR = PTMR__PTM__FRAME_CHANGE;

  int dir = 1;
  int ratio = 0;
  while (1) {
    v_blank_in();

    // increment/decrement the color calculation ratio once every 4 frames
    vdp2.reg.CCRSA = CCRSA__S0CCRT(ratio >> 2);

    ratio += dir;
    if (ratio >= (32 << 2) || ratio < 0) {
      dir = -dir;
      ratio += dir;
    }
  }
}
