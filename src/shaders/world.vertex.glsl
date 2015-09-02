layout(location = 0) in vec3 model;

uniform mat4 world_transform;
uniform mat4 vp_transform;

void main()
{
  vec4 world = world_transform * vec4(model, 1.);
  vec4 clip = vp_transform * world;
  gl_Position = clip;
}
