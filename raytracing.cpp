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

enum class light_type {
  ambient,
  point,
  directional
};

struct light {
  light_type type;
  fp16_16 intensity;
  union {
    vec3 position;
    vec3 direction;
  };
};

struct scene {
  sphere spheres[4];
  light lights[3];
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
    },
    {
      {0, -61, 0},
      fp16_16(60),
      {1, 1, 0},
    }
  },
  { // lights
    {
      light_type::ambient, // type
      fp16_16(65536 * 0.2, fp_raw_tag{}),        // intensity
      {{0, 0, 0}}          //
    },
    {
      light_type::point,   // type
      fp16_16(65536 * 0.6, fp_raw_tag{}),        // intensity
      {{2, 1, 0}}          // position
    },
    {
      light_type::directional, // type
      fp16_16(65536 * 0.6, fp_raw_tag{}),        // intensity
      {{1, 4, 4}}          // direction
    }
  }
};

static_assert(scene.spheres[0].center.z.value == (3 << 16));
static_assert(scene.lights[0].intensity.value != 0);
static_assert(scene.lights[1].position.x.value == (2 << 16));

struct t1_t2 {
  fp16_16 t1;
  fp16_16 t2;
};

fp16_16 compute_lighting(const vec3& point, const vec3& normal)
{
  fp16_16 intensity{0};

  for (int i = 0; i < 3; i++) {
    const light& light = scene.lights[i];
    if (light.type == light_type::ambient) {
      intensity += light.intensity;
    } else {
      vec3 light_vector;
      if (light.type == light_type::point) {
        light_vector = light.position - point;
      } else {
        light_vector = light.direction;
      }
      auto n_dot_l = dot(normal, light_vector);
      if (n_dot_l > fp16_16(0)) {
        intensity += light.intensity * n_dot_l * (fp16_16(1) / length(light_vector));
      }
    }
  }
  return intensity;
}

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
  for (int i = 0; i < 4; i++) {
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
    vec3 point = origin + direction * closest_t;
    vec3 normal = point - closest_sphere->center;
    normal = normal * (fp16_16(1) / length(normal));
    return closest_sphere->color * compute_lighting(point, normal);
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
