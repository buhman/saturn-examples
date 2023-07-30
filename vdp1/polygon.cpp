#include <stdint.h>
#include "vdp2.h"
#include "vdp1.h"

void main()
{
  // DISP: Please make sure to change this bit from 0 to 1 during V blank.
  vdp2.reg.TVMD = ( TVMD__DISP | TVMD__LSMD__NON_INTERLACE
                  | TVMD__VRESO__240 | TVMD__HRESO__NORMAL_320);

  // VDP2 User's Manual:
  // "When sprite data is in an RGB format, sprite register 0 is selected"
  // "When the value of a priority number is 0h, it is read as transparent"
  //
  // The power-on value of PRISA is zero. Set the priority for sprite register 0
  // to some number greater than zero, so that the color data is not interpreted
  // as "transparent".
  vdp2.reg.PRISA = PRISA__S0PRIN(1); // Sprite register 0 Priority Number

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

  constexpr uint16_t magenta = (31 << 10) | (31 << 0);
  vdp1.vram.cmd[2].CTRL = CTRL__JP__JUMP_NEXT | CTRL__COMM__POLYGON;
  vdp1.vram.cmd[2].LINK = 0;
  // "Set [ECD] to '1' for polygons, polylines, and lines"
  // "Be sure to set [SPD] to '1' for polygons, polylines, and lines"
  //
  // The "user clip mode" bit is not set in PMOD here, so setting "user clip
  // coordinates" has no effect on this draw command. However, "system clip
  // coordinates" and "local coordinates" are always applied, and must be set to
  // reasonable values.
  vdp1.vram.cmd[2].PMOD = PMOD__ECD | PMOD__SPD;
  vdp1.vram.cmd[2].COLR = COLR__RGB | magenta; // non-palettized (rgb15) color data
  vdp1.vram.cmd[2].XA = 100;
  vdp1.vram.cmd[2].YA = 50;
  vdp1.vram.cmd[2].XB = 150;
  vdp1.vram.cmd[2].YB = 100;
  vdp1.vram.cmd[2].XC = 100;
  vdp1.vram.cmd[2].YC = 150;
  vdp1.vram.cmd[2].XD = 50;
  vdp1.vram.cmd[2].YD = 100;

  vdp1.vram.cmd[3].CTRL = CTRL__END;

  // start drawing (execute the command list) on every frame
  vdp1.reg.PTMR = PTMR__PTM__FRAME_CHANGE;
}
