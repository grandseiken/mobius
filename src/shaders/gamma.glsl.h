const float gamma_value = 2.2;

vec3 ldr_tonemap(vec3 x)
{
  return clamp(x, 0., 1.);
}

vec3 reinhard_tonemap(vec3 x)
{
  return x / (x + 1);
}

vec3 exposure_tonemap(vec3 x)
{
  return vec3(1.) - exp2(-x);
}

vec3 gamma_decorrect(vec3 colour)
{
  return pow(colour, vec3(gamma_value));
}

vec3 gamma_correct(vec3 colour)
{
  return pow(colour, vec3(1. / gamma_value));
}
