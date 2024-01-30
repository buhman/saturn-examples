#include <stdint.h>
#include "vdp2.h"
#include "vdp1.h"
#include "scu.h"
#include "sh2.h"

extern void * _mai_data_pal_start __asm("_binary_res_mai_data_pal_start");
extern void * _mai_data_pal_size __asm("_binary_res_mai_data_pal_size");

extern void * _mai_data_start __asm("_binary_res_mai_data_start");
extern void * _mai_data_size __asm("_binary_res_mai_data_size");

inline constexpr uint16_t rgb15(const uint8_t * rgb24)
{
  return ((rgb24[2] >> 3) << 10) // blue
       | ((rgb24[1] >> 3) << 5)  // green
       | ((rgb24[0] >> 3) << 0); // red
}

uint32_t color_lookup_table(const uint32_t top)
{
  const uint32_t buf_size = reinterpret_cast<uint32_t>(&_mai_data_pal_size);
  if (buf_size != (0x20 * 3 / 2)) while (1); // halt if buf_size is incorrect

  const uint8_t * buf = reinterpret_cast<uint8_t*>(&_mai_data_pal_start);

  // "The size of a color lookup table is 20H (32) bytes"
  // (assume top is already aligned to 0x20)
  const uint32_t table_address = top - 0x20;

  // "The color lookup table defines the respective color codes of 16 colors in
  // VRAM as 16-bit data"
  uint16_t * table = &vdp1.vram.u16[(table_address / 2)];

  uint32_t buf_ix = 0;
  for (uint32_t i = 0; i < (buf_size / 3); i++) {
    // there is a typo in "5.2 Color Lookup Tables" "If RGB code, MSB = 0"
    // should be "MSB = 1". The "MSB = 0" claim is correctly contradicted later.

    table[i] = 1 << 15 | rgb15(&buf[buf_ix]);
    // _mai_data_pal is rgb24, 3 bytes per color
    buf_ix += 3;
  }

  return table_address;
}

uint32_t character_pattern_table(const uint32_t top)
{
  const uint32_t buf_size = reinterpret_cast<uint32_t>(&_mai_data_size);
  const uint32_t * buf = reinterpret_cast<uint32_t*>(&_mai_data_start);

  // Unlike vdp2 cell format, vdp1 sprites appear to be much more dimensionally
  // flexible. The data is interpreted as a row-major packed array, where the
  // row/horizontal stride is equal to the sprite width (as configured in the
  // draw command). This is identical to how the input palette index data is
  // structured, in that the stride of the input image data is deliberately the
  // same as the stride of the vdp1 sprite.
  //
  // Divide `buf_size` by two because this is a conversion from (indexed color)
  // 8 bit pixels to 4 bit pixels. Round up to the nearest 0x20 (for an 8000
  // pixel/8000 byte image, this rounding is a no-op).
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

constexpr uint32_t sprite_width = 72;
constexpr uint32_t sprite_height = 100;
constexpr uint32_t sprite_last_frame = 15;

static uint32_t color_address, character_address;
static uint32_t animation_timer;
static uint32_t sprite_frame_index;

// __attribute__ ((interrupt_handler)) changes code generation behavior for this
// function works. `rts` as generated in normal functions is replaced with
// `rte`.
void v_blank_in(void) __attribute__ ((interrupt_handler));
void v_blank_in(void)
{
  // clear V_BLANK_IN interrupt status
  scu.reg.IST &= ~(IST__V_BLANK_IN);
  scu.reg.IMS = ~(IMS__V_BLANK_IN);

  if (++animation_timer < 6)
    return;
  else
    animation_timer = 0;

  // 4 bits == 0.5 bytes per pixel
  constexpr uint32_t sprite_size = (sprite_width * sprite_height) / 2;

  sprite_frame_index = sprite_frame_index == sprite_last_frame ? 0 : sprite_frame_index + 1;
  vdp1.vram.cmd[2].SRCA = SRCA(character_address + (sprite_size * sprite_frame_index));
}

void main()
{
  uint32_t top = (sizeof (union vdp1_vram));
  top = color_address     = color_lookup_table(top);
  top = character_address = character_pattern_table(top);

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

  vdp1.vram.cmd[2].CTRL = CTRL__JP__JUMP_NEXT | CTRL__COMM__NORMAL_SPRITE;
  vdp1.vram.cmd[2].LINK = 0;
  // The "end code" is 0xf, which is being used in the mai sprite palette. If
  // both transparency and end codes are enabled, it seems there are only 14
  // usable colors in the 4-bit color mode.
  vdp1.vram.cmd[2].PMOD = PMOD__ECD | PMOD__COLOR_MODE__LOOKUP_TABLE_16;
  // It appears Kronos does not correctly calculate the color address in the
  // VDP1 debugger. Kronos will report FFFC when the actual color table address
  // in this example is 7FFE0.
  vdp1.vram.cmd[2].COLR = COLR__LOOKUP_TABLE__ADDRESS(color_address); // non-palettized (rgb15) color data
  vdp1.vram.cmd[2].SRCA = SRCA(character_address);
  vdp1.vram.cmd[2].SIZE = SIZE__X(sprite_width) | SIZE__Y(sprite_height);
  vdp1.vram.cmd[2].XA = 100;
  vdp1.vram.cmd[2].YA = 100;

  vdp1.vram.cmd[3].CTRL = CTRL__END;

  // start drawing (execute the command list) on every frame
  vdp1.reg.PTMR = PTMR__PTM__FRAME_CHANGE;

  //
  sprite_frame_index = 0;
  animation_timer = 0;
  sh2_vec[SCU_VEC__V_BLANK_IN] = reinterpret_cast<uint32_t>(&v_blank_in);

  // reset/enable V_BLANK_IN interrupt
  scu.reg.IST = 0;
  scu.reg.IMS = ~(IMS__V_BLANK_IN);
}
