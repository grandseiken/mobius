#include "simplex.glsl.h"

out vec4 output_colour;

uniform float amount;
uniform float frame;

void main()
{
  float time = frame / 32.;
  vec3 seed = vec3(gl_FragCoord.xy / 4., 0.);

  float n = 0.;
  n += simplex3(seed - vec3(0., 0., time / 3.375));
  n += 2. * simplex3(2. * seed - vec3(0., 0., time / 2.25));
  n += 4. * simplex3(4. * seed - vec3(0., 0., time / 1.5));
  n /= 4.;
  float v = clamp((n + 1.) / 2., 0., 1.);
  output_colour = vec4(v, v, v, amount);
}

