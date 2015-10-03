const float gamma_value = 2.2;

vec3 ldr_tonemap(vec3 x)
{
  return clamp(x, 0., 1.);
}

float ldr_tonemap(float x)
{
  return clamp(x, 0., 1.);
}

vec3 reinhard_tonemap(vec3 x)
{
  return x / (x + 1);
}

float reinhard_tonemap(float x)
{
  return x / (x + 1);
}

vec3 exposure_tonemap(vec3 x)
{
  return vec3(1.) - exp2(-x);
}

float exposure_tonemap(float x)
{
  return 1. - exp2(-x);
}

vec3 gamma_decorrect(vec3 colour)
{
  return pow(colour, vec3(gamma_value));
}

float gamma_decorrect(float colour)
{
  return pow(colour, gamma_value);
}

vec3 gamma_correct(vec3 colour)
{
  return pow(colour, vec3(1. / gamma_value));
}

float gamma_correct(float colour)
{
  return pow(colour, 1. / gamma_value);
}
