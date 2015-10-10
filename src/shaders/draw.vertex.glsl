layout(location = 0) in vec3 model;
layout(location = 1) in vec3 normal;
layout(location = 2) in float hue;
layout(location = 3) in float hue_shift;

smooth out vec3 vertex_world;
flat out vec3 vertex_normal;
flat out float vertex_hue;
flat out float vertex_hue_shift;

uniform mat3 normal_transform;
uniform mat4 world_transform;
uniform mat4 vp_transform;

uniform vec3 clip_points[8];
uniform vec3 clip_normals[8];

void main()
{
  vec3 world_normal = normal_transform * normal;
  vec4 world = world_transform * vec4(model, 1.);
  vec4 clip = vp_transform * world;
  gl_Position = clip;

  vertex_world = world.xyz;
  vertex_normal = normalize(world_normal);
  vertex_hue = hue;
  vertex_hue_shift = hue_shift;

  // Custom clipping planes.
  for (int i = 0; i < 8; ++i) {
    gl_ClipDistance[i] = dot(clip_normals[i], world.xyz - clip_points[i]);
  }
}
