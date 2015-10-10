#include "gamma.glsl.h"
#include "simplex.glsl.h"
#include "hsv.glsl.h"

out vec4 output_colour;

uniform float frame;
uniform vec2 dimensions;
uniform sampler2D read_framebuffer;
uniform sampler1D simplex_gradient_lut;
uniform sampler1D simplex_permutation_lut;
uniform bool simplex_use_permutation_lut;

// [0, 1].
const float grain_amount = 1. / 24;
// [0, 360).
const float hue_start = 340.;
// [0, 50].
const float hue_shift = 20.;
// [-50, 50].
const float saturation = -12.;

const int ramp_size = 16;
vec3 ramp_colour(int i, float source_hue)
{
  i = ramp_size - 1 - i;
  float hue =
      abs(mod(hue_start + source_hue * 360. - hue_shift * (i - 8.), 360));
  float shift = (8. - abs(i - 7.)) * saturation / 5.;
  return hsv_to_rgb(
      hue / 360., float(i) / (ramp_size - 1.) + shift / 100.,
      float(ramp_size - i - 1.) / (ramp_size - 1.) + shift / 100.);
}

vec3 ramp_colour(float v, float hue)
{
  float f = v * (ramp_size - 1.);
  int a = int(floor(f));
  int b = int(ceil(f));
  float t = mod(f, 1.);
  return ramp_colour(a, hue) * (1. - t) + ramp_colour(b, hue) * t;
}

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

  const float scale = .85;
  vec4 source = texture(read_framebuffer, gl_FragCoord.xy / dimensions);
  float source_intensity = scale * gamma_correct(source.x);
  float source_hue = source.y;

  float dynamic_grain_amount = grain_amount * (1 - source_intensity);
  float value = v * dynamic_grain_amount +
      (1 - dynamic_grain_amount) * source_intensity;

  output_colour = vec4(ramp_colour(value, source_hue), 1.);
}

