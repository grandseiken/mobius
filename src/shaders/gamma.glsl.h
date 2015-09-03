const float gamma_value = 2.2;

vec3 ldr_tonemap(vec3 x)
{
  return clamp(x, 0., 1.);
}

vec3 reinhard_tonemap(vec3 x)
{
  return x / (x + 1);
}

vec3 filmic_tonemap(vec3 x)
{
  float a = .15;
  float b = .5;
  float c = .1;
  float d = .2;
  float e = .02;
  float f = .3;
  return ((x * (a * x + b * c) + d * e) /
          (x * (a * x + b) + d * f)) - e / f;
}

vec3 gamma_decorrect(vec3 colour)
{
  return pow(colour, vec3(gamma_value));
}

vec3 gamma_correct(vec3 colour)
{
  return pow(colour, vec3(1. / gamma_value));
}
