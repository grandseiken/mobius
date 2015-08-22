layout(location = 0) in vec4 model;
layout(location = 1) in vec3 normal;

smooth out vec3 vertex_world;
flat out vec3 vertex_colour;
flat out vec3 vertex_normal;

uniform vec3 colour;
uniform mat3 normal_transform;
uniform mat4 world_transform;
uniform mat4 vp_transform;

void main()
{
  vec3 world_normal = normal_transform * normal;
  vec4 world = world_transform * model;
  vec4 clip = vp_transform * world;
  gl_Position = clip;

  vertex_world = world.xyz;
  vertex_normal = normalize(world_normal);
  vertex_colour = colour;
}
