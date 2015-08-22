#include "simplex.glsl.h"

out vec4 output_colour;

uniform float amount;
uniform float frame;

float simplex_layer(vec3 seed, float time, float pow)
{
  return simplex3(seed / pow - vec3(0., 0., time / pow)) / pow;
}

void main()
{
  float time = frame / 16.;
  vec3 seed = vec3(gl_FragCoord.xy / 2., gl_FragCoord.x / 8.);

  float n =
      simplex_layer(seed, time, 1.) +
      simplex_layer(seed, time, 2.) +
      simplex_layer(seed, time, 4.) + simplex_layer(seed, time, 8.);
  float v = clamp((n + 1.) / 2., 0., 1.);
  output_colour = vec4(v, v, v, v * amount);
}

