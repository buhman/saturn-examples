#include <stdint.h>
#include "vdp2.h"
#include "vdp1.h"

#include <concepts>

#include "common/vdp2_func.hpp"

#include "math/fp.hpp"
#include "math/vec3.hpp"
#include "math/mat4x4.hpp"

#include "cos.hpp"

#include "model/model.h"
#include "model/bear/material.h"
#include "model/bear/model.h"

// |--
// |
// |

using vec3 = vec<3, fp16_16>;
using mat4x4 = mat<4, 4, fp16_16>;

struct canvas {
  fp16_16 width;
  fp16_16 height;
};

constexpr struct canvas canvas = { 240, 240 };

template <typename T>
vec<3, T> viewport_to_canvas(T x, T y)
{
  return vec<3, T>(x * canvas.width, y * canvas.height, T(1));
}

template <typename T>
inline constexpr vec<3, T> project_vertex(vec<3, T> const& v)
{
  // / (v.z - T(5))
  // / (v.z - T(5))
  return viewport_to_canvas<T>((v.x * T(0.5) + T(2.0/3.0)),
			       (v.y * T(0.5) + T(0.5)));
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

static inline void render_quad(int ix, const vec3 a, const vec3 b, const vec3 c, const vec3 d)
{
  vdp1.vram.cmd[ix].CTRL = CTRL__JP__JUMP_NEXT | CTRL__COMM__POLYGON;
  vdp1.vram.cmd[ix].LINK = 0;
  vdp1.vram.cmd[ix].PMOD = PMOD__ECD | PMOD__SPD;
  vdp1.vram.cmd[ix].COLR = COLR__RGB | colors[ix & 3];

  vdp1.vram.cmd[ix].A.X = static_cast<int>(a.x);
  vdp1.vram.cmd[ix].A.Y = static_cast<int>(a.y);
  vdp1.vram.cmd[ix].B.X = static_cast<int>(b.x);
  vdp1.vram.cmd[ix].B.Y = static_cast<int>(b.y);
  vdp1.vram.cmd[ix].C.X = static_cast<int>(c.x);
  vdp1.vram.cmd[ix].C.Y = static_cast<int>(c.y);
  vdp1.vram.cmd[ix].D.X = static_cast<int>(d.x);
  vdp1.vram.cmd[ix].D.Y = static_cast<int>(d.y);
}

int render_object(int ix,
                  const mat4x4& screen,
                  const struct model * model,
                  const struct object * object0)
{
  mat4x4 trans = screen;

  for (int i = 0; i < object0->quadrilateral_count; i++) {

    const union quadrilateral * quad0 = &object0->quadrilateral[i];

    vec3 a0 = model->position[quad0->v[0].position];
    vec3 b0 = model->position[quad0->v[1].position];
    vec3 c0 = model->position[quad0->v[2].position];
    vec3 d0 = model->position[quad0->v[3].position];

    vec3 a = trans * a0;
    vec3 b = trans * b0;
    vec3 c = trans * c0;
    vec3 d = trans * d0;

    vec3 an0 = trans * model->normal[quad0->v[0].normal];
    //vec3 l = {0, 0, 1};
    fp16_16 cull = dot(a, an0);

    if (cull.value > 0) {
      // `origin` above is p==0 below; the `origin` calculation could
      // be reused, though it would hurt readability slightly
      /*
      for (int p = 0; p < 4; p++) {
	const vec3& v0 = vertices[face[p]];

	// rotation
	const vec3 v1 = transform * v0;
	// translation
	const vec3 v2{v1.x, v1.y + fp16_16(1), v1.z + fp16_16(2)};

	const vec3 v3 = project_vertex(v2 / v2.z);
      }
      */

      render_quad(ix,
                  project_vertex(a / a.z),
                  project_vertex(b / b.z),
                  project_vertex(c / c.z),
                  project_vertex(d / d.z));

      ix++;
    }
  }

  return ix;
}

void render()
{
  tick++;
  int ix = 2;

  const mat4x4 scale {
    1, 0, 0, 0,
    0, -1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
  };

  const int rx = tick >> 2;
  const mat4x4 rotationX {
    1,        0,       0, 0,
    0,  cos(rx), sin(rx), 0,
    0, -sin(rx), cos(rx), 0,
    0,        0,       0, 1,
  };

  const int ry = tick >> 1;
  const mat4x4 rotationY {
    cos(ry), 0, -sin(ry), 0,
          0, 1,        0, 0,
    sin(ry), 0,  cos(ry), 0,
          0, 0,        0, 1,
  };

  const mat4x4 translation = {
    1, 0, 0, 0,
    0, 1, 0, 4.5,
    0, 0, 1, 6.5,
    0, 0, 0, 1,
  };

  const mat4x4 screen = translation * rotationY * scale;

  //const vec3 camera = {0, 0, 0};

  const int frame_ix0 = 0;
  const struct model * model = &bear_model;
  const struct object * object0 = bear_object[frame_ix0 * 2];
  //const struct object * object1 = bear_object[frame_ix1 * 2];

  ix = render_object(ix, screen, model, object0);

  vdp1.vram.cmd[ix].CTRL = CTRL__END;
}

void main()
{
  v_blank_in();

  // DISP: Please make sure to change this bit from 0 to 1 during V blank.
  vdp2.reg.TVMD = ( TVMD__DISP | TVMD__LSMD__NON_INTERLACE
                  | TVMD__VRESO__240 | TVMD__HRESO__NORMAL_320);

  // disable all VDP2 backgrounds (e.g: the Sega bios logo)
  vdp2.reg.BGON = 0;

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
