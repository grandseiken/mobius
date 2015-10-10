#include "hsv.glsl.h"
layout(location = 0) in vec3 world;
layout(location = 1) in vec3 colour;

smooth out vec3 vertex_world;
flat out float vertex_hue;

uniform mat4 world_transform;
uniform mat4 vp_transform;

uniform vec3 clip_points[8];
uniform vec3 clip_normals[8];

void main()
{
  vec4 clip = vp_transform * vec4(world, 1.);
  gl_Position = clip;

  vertex_world = world;
  vertex_hue = rgb_to_hue(colour);

  // Custom clipping planes.
  for (int i = 0; i < 8; ++i) {
    gl_ClipDistance[i] = dot(clip_normals[i], world.xyz - clip_points[i]);
  }
}
