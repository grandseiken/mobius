#include "simplex.glsl.h"

out vec4 output_colour;

uniform float frame;
uniform vec2 dimensions;
uniform sampler2D read_framebuffer;
uniform sampler1D simplex_gradient_lut;
uniform sampler1D simplex_permutation_lut;
uniform bool simplex_use_permutation_lut;

vec3 hsv_to_rgb(float h, float s, float v)
{
  h = mod(h, 360.) / 60.;
  s = clamp(s, 0., 1.);
  v = clamp(v, 0., 1.);
  if (v == 0.) {
    return vec3(0.);
  }
  float i = floor(h);
  float f = h - i;
  float p = v * (1. - s);
  float q = v * (1. - (s * f));
  float t = v * (1. - (s * (1. - f)));

  return i == 0 ? vec3(v, t, p) :
         i == 1 ? vec3(q, v, p) :
         i == 2 ? vec3(p, v, t) :
         i == 3 ? vec3(p, q, v) :
         i == 4 ? vec3(t, p, v) :
         i == 5 ? vec3(v, p, q) :
                  vec3(0., 0., 0.);
}

// [0, 1].
const float grain_amount = 1. / 24;
// [0, 360).
const float hue_start = 340;
// [0, 50].
const float hue_shift = 20;
// [-50, 50].
const float saturation = -10;

const int ramp_size = 16;
vec3 ramp_colour(int i)
{
  i = ramp_size - 1 - i;
  float hue = abs(mod(hue_start - hue_shift * (i - 8.), 360));
  float shift = (8. - abs(i - 7.)) * saturation / 5.;
  return hsv_to_rgb(
      hue, float(i) / (ramp_size - 1.) + shift / 100.,
      float(ramp_size - i - 1.) / (ramp_size - 1.) + shift / 100.);
}

vec3 ramp_colours[ramp_size] = vec3[](
  ramp_colour(0), ramp_colour(1), ramp_colour(2), ramp_colour(3),
  ramp_colour(4), ramp_colour(5), ramp_colour(6), ramp_colour(7),
  ramp_colour(8), ramp_colour(9), ramp_colour(10), ramp_colour(11),
  ramp_colour(12), ramp_colour(13), ramp_colour(14), ramp_colour(15));

vec3 ramp_colour(float v)
{
  float f = v * (ramp_size - 1.);
  int a = int(floor(f));
  int b = int(ceil(f));
  float t = mod(f, 1.);
  return ramp_colours[a] * (1. - t) + ramp_colours[b] * t;
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

  float source = texture(read_framebuffer, gl_FragCoord.xy / dimensions).x;
  float value = v * v * grain_amount + (1 - v * grain_amount) * source;
  output_colour = vec4(ramp_colour(value), 1.);
}

