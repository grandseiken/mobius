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
// [-50, 50].
const float saturation = -12.;

const int ramp_size = 16;
vec3 ramp_colour(int i, float source_hue, float source_hue_shift)
{
  i = ramp_size - 1 - i;
  float hue =
      abs(mod(source_hue * 360. - source_hue_shift * 50. * (i - 8.), 360));
  float shift = (8. - abs(i - 7.)) * saturation / 5.;
  return hsv_to_rgb(
      hue / 360., float(i) / (ramp_size - 1.) + shift / 100.,
      float(ramp_size - i - 1.) / (ramp_size - 1.) + shift / 100.);
}

vec3 ramp_colour(float v, float source_hue, float source_hue_shift)
{
  float f = v * (ramp_size - 1.);
  int a = int(floor(f));
  int b = int(ceil(f));
  float t = mod(f, 1.);
  return ramp_colour(a, source_hue, source_hue_shift) *
      (1. - t) + ramp_colour(b, source_hue, source_hue_shift) * t;
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
  float source_intensity = source.x;
  float source_hue = source.y;
  float source_hue_shift = source.z;

  float dynamic_grain_amount = grain_amount * (1 - source_intensity);
  float value = v * dynamic_grain_amount +
      (1 - dynamic_grain_amount) * scale * gamma_correct(source_intensity);

  output_colour =
      vec4(ramp_colour(value, source_hue, source_hue_shift), 1.);
}

