#include <stdint.h>
#include "vdp2.h"
#include "vdp1.h"

struct gimp_image {
  uint8_t width;
  uint8_t height;
  uint8_t bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
  uint8_t pixel_data[48 * 48 * 3 + 1];
};

extern struct gimp_image chikorita;

inline uint16_t rgb15(const uint8_t * rgb24)
{
  return ((rgb24[2] >> 3) << 10) // blue
       | ((rgb24[1] >> 3) << 5)  // green
       | ((rgb24[0] >> 3) << 0); // red
}

uint32_t character_pattern_table(const uint32_t top)
{
  const uint32_t image_size = chikorita.width * chikorita.height * 2; // in bytes of vdp1 vram
  const uint32_t table_size = ((image_size) + 0x20 - 1) & (-0x20); // round up to the nearest multiple of 32
  const uint32_t table_address = top - table_size;
  uint16_t * table = &vdp1.vram.u16[(table_address / 2)];

  for (int32_t y = 0; y < (int32_t)chikorita.height; y++) {
    for (int32_t x = 0; x < (int32_t)chikorita.width; x++) {
      int32_t pixel_index = (y * chikorita.width + x);
      const uint8_t * rgb24 = &chikorita.pixel_data[pixel_index * 3];
      table[pixel_index] = COLR__RGB | rgb15(rgb24);
    }
  }

  return table_address;
}

void main()
{
  uint32_t character_address;
  uint32_t top = (sizeof (union vdp1_vram));
  top = character_address = character_pattern_table(top);

  // DISP: Please make sure to change this bit from 0 to 1 during V blank.
  vdp2.reg.TVMD = ( TVMD__DISP | TVMD__LSMD__NON_INTERLACE
                  | TVMD__VRESO__240 | TVMD__HRESO__NORMAL_320);

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
  const uint16_t black = 0x0000;
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
  vdp1.vram.cmd[2].PMOD = PMOD__COLOR_MODE__RGB;
  vdp1.vram.cmd[2].COLR = 0;
  vdp1.vram.cmd[2].SRCA = character_address >> 3;
  vdp1.vram.cmd[2].SIZE = SIZE__X(chikorita.width) | SIZE__Y(chikorita.height);
  vdp1.vram.cmd[2].XA = 50;
  vdp1.vram.cmd[2].YA = 50;

  vdp1.vram.cmd[3].CTRL = CTRL__END;

  // start drawing (execute the command list) on every frame
  vdp1.reg.PTMR = PTMR__PTM__FRAME_CHANGE;
}
