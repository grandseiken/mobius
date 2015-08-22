layout(location = 0) in vec4 model;
layout(location = 1) in vec3 normal;

smooth out vec3 vertex_world;
flat out vec3 vertex_colour;
flat out vec3 vertex_normal;

uniform vec3 colour;
uniform mat4 world_transform;
uniform mat4 vp_transform;

void main()
{
  vec4 world = world_transform * model;
  vec4 world_normal = world_transform * vec4(normal, 0.);
  vec4 clip = vp_transform * world;
  gl_Position = clip;
  vertex_world = world.xyz;
  vertex_normal = normalize(world_normal.xyz);
  vertex_colour = colour;
}
