#include "gamma.glsl.h"
smooth in vec3 vertex_world;
flat in float vertex_hue;

out vec4 output_colour;

uniform vec3 light_source;

void main()
{
  vec3 light_difference = light_source - vertex_world;
  float light_distance_sq = dot(light_difference, light_difference);

  float intensity = 2. / (1. + light_distance_sq);
  output_colour = vec4(reinhard_tonemap(intensity), vertex_hue, 0., 1.);
}

