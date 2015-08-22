smooth in vec3 vertex_world;
flat in vec3 vertex_colour;
flat in vec3 vertex_normal;

out vec4 output_colour;

uniform vec3 light_source;
uniform float light_intensity;

const float gamma = 2.2;
const vec3 gamma_uncorrect = vec3(gamma);
const vec3 gamma_correct = vec3(1. / gamma);

void main()
{
  vec3 light_difference = light_source - vertex_world;
  float light_distance_sq = dot(light_difference, light_difference);
  vec3 light_normal = light_difference * inversesqrt(light_distance_sq);

  float cos_angle = dot(light_normal, vertex_normal);
  float intensity = (light_intensity * cos_angle) / (1. + light_distance_sq);

  const float ambient = .1;
  intensity = clamp(ambient + intensity, 0., 1.);

  vec3 lit_colour =
      pow(intensity * pow(vertex_colour, gamma_uncorrect), gamma_correct);
  output_colour = vec4(lit_colour, 1.);
}

