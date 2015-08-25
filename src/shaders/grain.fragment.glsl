#include "simplex.glsl.h"

out vec4 output_colour;

uniform float amount;
uniform float frame;
uniform sampler1D simplex_gradient_lut;
uniform sampler1D simplex_permutation_lut;

float simplex_layer(vec3 seed, float time, float pow)
{
  vec3 scaled_seed = seed / pow - vec3(0., 0., time / pow);
  return (1. / pow) *
      simplex3(scaled_seed, simplex_gradient_lut, simplex_permutation_lut);
}

void main()
{
  float time = frame / 16.;
  vec3 seed = vec3(gl_FragCoord.xy / 2., gl_FragCoord.x / 8.);

  float n =
      simplex_layer(seed, time, 1.) +
      simplex_layer(seed, time, 2.) +
      simplex_layer(seed, time, 4.);
  float v = clamp((n + 1.) / 2., 0., 1.);
  output_colour = vec4(v, v, v, v * amount);
}

