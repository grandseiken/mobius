smooth in vec3 vertex_world;
flat in vec3 vertex_colour;
flat in vec3 vertex_normal;

out vec4 output_colour;

uniform vec3 light_source;
uniform float light_intensity;

void main()
{
  float light_distance = distance(light_source, vertex_world);
  vec3 light_normal = normalize(light_source - vertex_world);

  float cos_angle = dot(light_normal, vertex_normal);
  float intensity =
      (light_intensity * cos_angle) /
      max(1., light_distance * light_distance);

  const float ambient = .1;
  intensity = clamp(ambient + intensity, 0., 1.);
  output_colour = vec4(intensity * vertex_colour, 1.);
}

