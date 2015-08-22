flat in vec3 vertex_colour;
flat in vec3 vertex_normal;

out vec4 output_colour;

void main()
{
  vec3 light_normal = normalize(vec3(3., -2., -1.));
  float cos_angle = max(0., dot(-vertex_normal, light_normal));
  output_colour = vec4(min(1., .1 + cos_angle) * vertex_colour, 1.);
}

