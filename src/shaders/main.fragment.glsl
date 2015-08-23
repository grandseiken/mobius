#include "gamma.glsl.h"
#include "simplex.glsl.h"

smooth in vec3 vertex_model;
smooth in vec3 vertex_world;
flat in vec3 vertex_colour;
flat in vec3 vertex_normal;

out vec4 output_colour;

uniform vec3 light_source;
uniform float light_intensity;

void main()
{
  vec3 light_difference = light_source - vertex_world;
  float light_distance_sq = dot(light_difference, light_difference);
  vec3 light_normal = light_difference * inversesqrt(light_distance_sq);

  float cos_angle = dot(light_normal, vertex_normal);
  float intensity = (light_intensity * cos_angle) / (1. + light_distance_sq);
  intensity = clamp(intensity, 0., 1.);

  float texture =
      simplex3(2048. * vertex_model) +
      simplex3(1024. * vertex_model) +
      simplex3(512. * vertex_model) +
      simplex3(256. * vertex_model) +
      simplex3(128. * vertex_model) +
      simplex3(64. * vertex_model) +
      simplex3(32. * vertex_model) +
      simplex3(16. * vertex_model) +
      simplex3(8. * vertex_model) +
      simplex3(4. * vertex_model);
  texture = (texture / 4. + 1.) / 2.;

  vec3 lit_colour = gamma_correct(
      intensity * texture * gamma_decorrect(vertex_colour));
  output_colour = vec4(lit_colour, 1.);
}

