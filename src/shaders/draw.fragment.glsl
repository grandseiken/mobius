#include "gamma.glsl.h"
#include "simplex.glsl.h"

smooth in vec3 vertex_world;
flat in vec3 vertex_colour;
flat in vec3 vertex_normal;

out vec4 output_colour;

uniform vec3 light_source;
uniform float light_intensity;
uniform sampler1D simplex_gradient_lut;
uniform sampler1D simplex_permutation_lut;
uniform bool simplex_use_permutation_lut;

const float mrot = 1. / 128;
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
  return scale * dF > 2 ? vec4(0.) :
      simplex3_gradient(
          scale * value, simplex_gradient_lut,
          simplex_use_permutation_lut, simplex_permutation_lut);
}

void main()
{
  // We rotate slightly to avoid planar cuts through 3D noise.
  vec3 seed = mrotm * vertex_world;
  float dF = dFmax(seed);
  vec4 texture = vec4(0.);
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

  vec3 plane0 = normalize(get_perpendicular(vertex_normal));
  vec3 plane1 = cross(vertex_normal, plane0);
  float grad0 = dot(texture.xyz, plane0);
  float grad1 = dot(texture.xyz, plane1);
  vec3 surface_normal = normalize(
      vertex_normal - grad0 * plane0 - grad1 * plane1);

  vec3 light_difference = light_source - vertex_world;
  float light_distance_sq = dot(light_difference, light_difference);
  vec3 light_normal = light_difference * inversesqrt(light_distance_sq);

  // When both light and camera are too close to the surface, we get artifacts
  // far away that show the plane and ruin the effect. We could fake occlusion
  // of back-facing normals somehow.
  float cos_angle = (dot(light_normal, surface_normal) +
                     dot(light_normal, vertex_normal)) / 2.;
  float intensity = (light_intensity * cos_angle) / (1. + light_distance_sq);
  intensity = clamp(intensity, 0., 1.);

  // Still don't think this gamma correction is quite right.
  vec3 lit_colour = gamma_correct(
      intensity * (texture.a + 1.) / 2. * gamma_decorrect(vertex_colour));
  output_colour = vec4(lit_colour, 1.);
}
