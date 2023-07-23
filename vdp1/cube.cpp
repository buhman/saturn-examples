#include <stdint.h>
#include "vdp2.h"
#include "vdp1.h"

#include <concepts>

#include "../common/vdp2_func.hpp"

#include "../math/fp.hpp"
#include "../math/vec4.hpp"
#include "../math/vec3.hpp"
#include "../math/mat4x4.hpp"

#include "cos.hpp"

// |--
// |
// |

using vec4 = vec<4, fp16_16>;
using vec3 = vec<3, fp16_16>;
using mat4x4 = mat<4, 4, fp16_16>;

static vec4 vertices[8] = {
  {-1.0, -1.0,  1.0, 1.0}, // top left front
  { 1.0, -1.0,  1.0, 1.0}, // top right front
  { 1.0,  1.0,  1.0, 1.0}, // bottom right front
  {-1.0,  1.0,  1.0, 1.0}, // bottom left front

  {-1.0, -1.0, -1.0, 1.0}, // top left back
  { 1.0, -1.0, -1.0, 1.0}, // top right back
  { 1.0,  1.0, -1.0, 1.0}, // bottom right back
  {-1.0,  1.0, -1.0, 1.0}, // bottom left back
};

static uint32_t faces[6][4] = {
  {0, 1, 2, 3}, // front clockwise
  {5, 4, 7, 6}, // back clockwise
  {0, 4, 5, 1}, // top clockwise
  {3, 2, 6, 7}, // bottom clockwise
  {4, 0, 3, 7}, // left clockwise
  {1, 5, 6, 2}, // right clockwise
};

struct canvas {
  fp16_16 width;
  fp16_16 height;
};

constexpr struct canvas canvas = { 320, 240 };

template <typename T>
vec<3, T> viewport_to_canvas(T x, T y)
{
  return vec<3, T>((canvas.width>>1) + x * canvas.height, (canvas.height>>1) - y * canvas.height - T(1), T(1));
}

template <typename T>
inline constexpr vec<3, T> project_vertex(vec<4, T> const& v)
{
  return viewport_to_canvas<T>((v.x / v.z), (v.y / v.z));
}

constexpr inline uint16_t rgb15(int32_t r, int32_t g, int32_t b)
{
  return ((b & 31) << 10) | ((g & 31) << 5) | ((r & 31) << 0);
}

constexpr uint16_t colors[] = {
  rgb15(31,  0,  0), // red
  rgb15( 0, 31,  0), // green
  rgb15( 0,  0, 31), // blue
  rgb15(31,  0, 31), // magenta
  rgb15( 0, 31, 31), // cyan
  rgb15(31, 31,  0), // yellow
};

static int32_t tick = 0;

void
render()
{
  tick++;
  int ix = 2;

  const int rx = tick >> 2;
  const mat4x4 rotationX {
    1,        0,       0, 0,
    0,  cos(rx), sin(rx), 0,
    0, -sin(rx), cos(rx), 0,
    0,        0,       0, 1,
  };

  const int ry = tick >> 2;
  const mat4x4 rotationY {
    cos(ry), 0, -sin(ry), 0,
          0, 1,        0, 0,
    sin(ry), 0,  cos(ry), 0,
          0, 0,        0, 1,
  };

  //const mat4x4 transform = rotationX * rotationY;

  vec4 transforms[2] = {
    {-1.5, 0.0, 7.0, 0.0},
    {1.25, 2, 7.5, 0.0}
  };

  for (vec4& t : transforms) {
    for (int i = 0; i < 6; i++) {

      const uint32_t * face = faces[i];

      vdp1.vram.cmd[ix].CTRL = CTRL__JP__JUMP_NEXT | CTRL__COMM__POLYLINE;
      vdp1.vram.cmd[ix].LINK = 0;
      vdp1.vram.cmd[ix].PMOD = PMOD__ECD | PMOD__SPD;
      vdp1.vram.cmd[ix].COLR = COLR__RGB | colors[i];

      for (int p = 0; p < 4; p++) {
        const vec4& v0 = vertices[face[p]];

        const vec4& v1 = v0 + t;

        const vec3& v2 = project_vertex(v1);

        vdp1.vram.cmd[ix].point[p].X = static_cast<int>(v2.x);
        vdp1.vram.cmd[ix].point[p].Y = static_cast<int>(v2.y);
      }
      ix++;
    }
  }

  vdp1.vram.cmd[ix].CTRL = CTRL__END;
}

void main()
{
  v_blank_in();

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

  vdp1.vram.cmd[2].CTRL = CTRL__END;

  // start drawing (execute the command list) on every frame
  vdp1.reg.PTMR = PTMR__PTM__FRAME_CHANGE;

  while (true) {
    v_blank_in();
    render();
  }
}

extern "C"
void start(void)
{
  main();
  while (1) {}
}
