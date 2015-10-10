#include "gamma.glsl.h"
#include "simplex.glsl.h"

smooth in vec3 vertex_world;
flat in vec3 vertex_normal;
flat in float vertex_hue;

out vec4 output_colour;

uniform vec3 light_source;
uniform sampler1D simplex_gradient_lut;
uniform sampler1D simplex_permutation_lut;
uniform bool simplex_use_permutation_lut;

const float mrot = 1. / 256;
const mat3 mrotm =
    mat3(1,         0,          0,
         0, cos(mrot), -sin(mrot),
         0, sin(mrot),  cos(mrot)) *
    mat3(cos(mrot), -sin(mrot), 0,
         sin(mrot),  cos(mrot), 0,
                 0,          0, 1);

// Gets some vertex perpendicular to this one.
vec3 get_perpendicular(vec3 v)
{
  return mix(
      vec3(v.z, v.z, -v.x - v.y),
      vec3(-v.y - v.z, v.x, v.x),
      float(v.z != 0 && -v.x != v.y));
}

float dFmax(vec3 value)
{
  vec3 world_dx = dFdx(value);
  vec3 world_dy = dFdy(value);
  return max(length(world_dx), length(world_dy));
}

vec4 dFsimplex3(float scale, float dF, vec3 value)
{
  // We assume the average over 2 units of noise is zero; this value could be
  // tweaked up or down.
  // TODO: this really needs smoothed.
  return scale * dF > 2 ? vec4(0.) :
      simplex3_gradient(
          scale * value, simplex_gradient_lut,
          simplex_use_permutation_lut, simplex_permutation_lut);
}

const bool rocky = false;
const float light_intensity = 64.;
const vec3 light_direction = normalize(vec3(1., 1.125, 1.25));

void main()
{
  // We rotate slightly to avoid planar cuts through 3D noise.
  vec3 seed = mrotm * vertex_world;
  float dF = dFmax(seed);
  vec4 texture = vec4(0.);
  if (rocky) {
    texture += dFsimplex3(2., dF, seed);
    texture += dFsimplex3(4., dF, seed);
    texture += dFsimplex3(8., dF, seed);
    texture += dFsimplex3(16., dF, seed);
    texture += dFsimplex3(32., dF, seed);
    texture += dFsimplex3(64., dF, seed);
    texture += dFsimplex3(128., dF, seed);
    texture += dFsimplex3(256., dF, seed);
    texture += dFsimplex3(512., dF, seed);
    texture += dFsimplex3(1024., dF, seed);
    texture += dFsimplex3(2048., dF, seed);
    texture += dFsimplex3(4096., dF, seed);
    texture = texture / 16.;
  }

  vec3 plane0 = normalize(get_perpendicular(vertex_normal));
  vec3 plane1 = cross(vertex_normal, plane0);
  float grad0 = dot(texture.xyz, plane0);
  float grad1 = dot(texture.xyz, plane1);
  vec3 surface_normal = normalize(
      vertex_normal - grad0 * plane0 - grad1 * plane1);

  vec3 light_difference = light_source - vertex_world;
  float light_distance_sq = dot(light_difference, light_difference);

  // Completely faked lighting.
  float cos_angle = (dot(light_direction, surface_normal) +
                     dot(light_direction, vertex_normal)) / 2.;
  cos_angle = .5 + cos_angle / 2.;

  float intensity = (light_intensity * cos_angle) / (1. + light_distance_sq);
  // Extremely simple HDR (tone-mapping). Works because we only do one render
  // pass; multiple passes would require either special HDR framebuffers or
  // dynamic aperture based on reading the brightness from the last frame.
  float lit_colour = intensity * (texture.a + 1.) / 2.;
  output_colour = vec4(reinhard_tonemap(lit_colour), vertex_hue, 0., 1.);
}

