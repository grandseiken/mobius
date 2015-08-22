layout(location = 0) in vec4 model;
layout(location = 1) in vec3 normal;

flat out vec3 vertex_colour;
flat out vec3 vertex_normal;

uniform vec3 colour;
uniform mat4 transform;

void main()
{
  vec4 clip = transform * model;
  gl_Position = clip;
  vertex_normal = normal;
  vertex_colour = colour;
}
