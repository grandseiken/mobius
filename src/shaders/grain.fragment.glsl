#include "noise3D.glsl"

out vec4 output_colour;

uniform float amount;
uniform float frame;

void main()
{
  float time = frame / 32;
  vec3 seed = vec3(gl_FragCoord.xy / 4, 0);

  float n = 0;
  n += snoise(seed - vec3(0, 0, time / 3.375));
  n += 2 * snoise(2 * seed - vec3(0, 0, time / 2.25));
  n += 4 * snoise(4 * seed - vec3(0, 0, time / 1.5));
  n += 8 * snoise(8 * seed - vec3(0, 0, time));
  n /= 8;
  float v = (n + 1) / 2;
  output_colour = vec4(v, v, v, amount);
}

