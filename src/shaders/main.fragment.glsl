#include "gamma.glsl.h"
#include "simplex.glsl.h"

smooth in vec3 vertex_model;
smooth in vec3 vertex_world;
flat in vec3 vertex_colour;
flat in vec3 vertex_normal;

out vec4 output_colour;

uniform vec3 light_source;
uniform float light_intensity;
uniform sampler1D simplex_gradient_lut;
uniform sampler1D simplex_permutation_lut;

float dFmax(vec3 value)
{
  vec3 world_dx = dFdx(value);
  vec3 world_dy = dFdy(value);
  return max(length(world_dx), length(world_dy));
}

float dFsimplex3(float scale, float dF, vec3 value)
{
  // We assume the average over 2 units of noise is zero; this value could be
  // tweaked up or down.
  return scale * dF > 2 ? 0. :
      simplex3(scale * value, simplex_gradient_lut, simplex_permutation_lut);
}

void main()
{
  vec3 light_difference = light_source - vertex_world;
  float light_distance_sq = dot(light_difference, light_difference);
  vec3 light_normal = light_difference * inversesqrt(light_distance_sq);

  float cos_angle = dot(light_normal, vertex_normal);
  float intensity = (light_intensity * cos_angle) / (1. + light_distance_sq);
  intensity = clamp(intensity, 0., 1.);

  float dF = dFmax(vertex_model);
  float texture =
      dFsimplex3(2., dF, vertex_model) +
      dFsimplex3(4., dF, vertex_model) +
      dFsimplex3(8., dF, vertex_model) +
      dFsimplex3(16., dF, vertex_model) +
      dFsimplex3(32., dF, vertex_model) +
      dFsimplex3(64., dF, vertex_model) +
      dFsimplex3(128., dF, vertex_model) +
      dFsimplex3(256., dF, vertex_model) +
      dFsimplex3(512., dF, vertex_model) +
      dFsimplex3(1024., dF, vertex_model) +
      dFsimplex3(2048., dF, vertex_model) +
      dFsimplex3(4096., dF, vertex_model);
  texture = (texture / 8. + 1.) / 2.;

  vec3 lit_colour = gamma_correct(
      intensity * texture * gamma_decorrect(vertex_colour));
  output_colour = vec4(lit_colour, 1.);
}

