layout(location = 0) in vec3 model;

smooth out vec3 vertex_world;

uniform mat4 world_transform;
uniform mat4 vp_transform;

uniform vec3 clip_points[8];
uniform vec3 clip_normals[8];

void main()
{
  vec4 world = world_transform * vec4(model, 1.);
  vec4 clip = vp_transform * world;
  gl_Position = clip;

  vertex_world = world.xyz;

  // Custom clipping planes.
  for (int i = 0; i < 8; ++i) {
    gl_ClipDistance[i] = dot(clip_normals[i], world.xyz - clip_points[i]);
  }
}
