layout(location = 0) in vec3 model;

uniform mat4 world_transform;
uniform mat4 vp_transform;

uniform vec3 clip_point;
uniform vec3 clip_normal;

void main()
{
  vec4 world = world_transform * vec4(model, 1.);
  vec4 clip = vp_transform * world;
  gl_Position = clip;

  // Custom clipping plane.
  gl_ClipDistance[0] = dot(clip_normal, world.xyz - clip_point);
}
