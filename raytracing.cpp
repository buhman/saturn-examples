#include <stdint.h>

#include "vec.hpp"
#include "fp.hpp"
#include "raytracing.hpp"

namespace viewport {
  constexpr int width = 1;
  constexpr int height = 1;
}

vec3 canvas_to_viewport(int cx, int cy)
{
  return vec3(
    fp16_16(((cx * viewport::width) * (1 << 16)) >> canvas::bit_width, fp_raw_tag{}),
    fp16_16(((cy * viewport::height) * (1 << 16)) >> canvas::bit_height, fp_raw_tag{}),
    fp16_16(1)
  );
}

struct sphere {
  vec3 center;
  fp16_16 radius;
  vec3 color;
};

struct scene {
  sphere spheres[3];
};

constexpr scene scene {
  { // spheres
    {
      {0, -1, 3}, // center
      fp16_16(1), // radius
      {1, 0, 0},  // color
    },
    {
      {2, 0, 4},
      fp16_16(1),
      {0, 0, 1},
    },
    {
      {-2, 0, 4},
      fp16_16(1),
      {0, 1, 0},
    }
  }
};

static_assert(scene.spheres[0].center.z.value == (3 << 16));

struct t1_t2 {
  fp16_16 t1;
  fp16_16 t2;
};

t1_t2 intersect_ray_sphere(const vec3& origin, const vec3& direction, const sphere& sphere)
{
  fp16_16 r = sphere.radius;
  vec3 CO = origin - sphere.center;

  auto a = dot(direction, direction);
  auto b = dot(CO, direction) * static_cast<int32_t>(2);
  auto c = dot(CO, CO) - r*r;

  auto discriminant = b*b - static_cast<int32_t>(4)*a*c;

  if (discriminant < fp16_16(0)) {
    return {fp_limits<fp16_16>::max(), fp_limits<fp16_16>::max()};
  } else {
    auto sqrt_d = sqrt(discriminant);
    auto a2 = fp16_16(a*static_cast<int32_t>(2));
    auto t1 = (-b + sqrt_d) / a2;
    auto t2 = (-b - sqrt_d) / a2;
    return {t1, t2};
  }
}

static vec3 trace_ray
(
  const vec3& origin,
  const vec3& direction,
  const fp16_16 t_min,
  const fp16_16 t_max
)
{
  fp16_16 closest_t = fp_limits<fp16_16>::max();
  const sphere * closest_sphere = nullptr;
  for (int i = 0; i < 3; i++) {
    auto& sphere = scene.spheres[i];
    auto [t1, t2] = intersect_ray_sphere(origin, direction, sphere);
    if (t1 >= t_min && t1 < t_max && t1 < closest_t) {
      closest_t = t1;
      closest_sphere = &sphere;
    }
    if (t2 >= t_min && t2 < t_max && t2 < closest_t) {
      closest_t = t2;
      closest_sphere = &sphere;
    }
  }
  if (closest_sphere == nullptr) {
    return vec3(0, 0, 0);
  } else {
    return closest_sphere->color;
  }
}

void render(void (&put_pixel) (int32_t x, int32_t y, const vec3& c))
{
  using namespace canvas;

  vec3 origin = vec3(0, 0, 0);

  for (int x = -(width/2); x < (width/2); x++) {
    for (int y = -(height/2 + 1); y < (height/2 + 1); y++) {
      vec3 direction = canvas_to_viewport(x, y);
      vec3 color = trace_ray(origin, direction,
                             fp16_16(1),
                             fp_limits<fp16_16>::max());
      put_pixel(x, y, color);
    }
  }
}
