const float gamma_value = 2.2;

vec3 gamma_decorrect(vec3 colour)
{
  return pow(colour, vec3(gamma_value));
}

vec3 gamma_correct(vec3 colour)
{
  return pow(colour, vec3(1. / gamma_value));
}
