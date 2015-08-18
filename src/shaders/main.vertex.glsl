layout(location = 0) in vec4 model;

flat out vec3 vertex_colour;

uniform vec3 colour;
uniform mat4 transform;

void main()
{
  vec4 clip = transform * model;
  gl_Position = clip;
  vertex_colour = colour;
}
