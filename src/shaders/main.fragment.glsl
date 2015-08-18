flat in vec3 vertex_colour;

out vec4 output_colour;

void main()
{
  output_colour = vec4(vertex_colour, 1);
}

