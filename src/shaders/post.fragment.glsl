#include "simplex.glsl.h"

out vec4 output_colour;

uniform float frame;
uniform vec2 dimensions;
uniform sampler2D read_framebuffer;
uniform sampler1D simplex_gradient_lut;
uniform sampler1D simplex_permutation_lut;
uniform bool simplex_use_permutation_lut;

const float grain_amount = 1. / 24;

float simplex_layer(vec3 seed, float time, float pow)
{
  vec3 scaled_seed = seed / pow - vec3(0., 0., time / pow);
  return (1. / pow) *
      simplex3(scaled_seed, simplex_gradient_lut,
               simplex_use_permutation_lut, simplex_permutation_lut);
}

void main()
{
  float time = frame / 32.;
  vec3 seed = vec3(gl_FragCoord.xy / 2., gl_FragCoord.x / 8.);

  float n =
      simplex_layer(seed, time, 1.) +
      simplex_layer(seed, time, 2.) +
      simplex_layer(seed, time, 4.);
  float v = clamp((n + 1.) / 2., 0., 1.);

  float source = texture(read_framebuffer, gl_FragCoord.xy / dimensions).x;
  float value = v * v * grain_amount + (1 - v * grain_amount) * source;
  output_colour = vec4(value, value, value, 1.);
}

