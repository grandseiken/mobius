vec3 hsv_to_rgb(float h, float s, float v)
{
  h = 6. * mod(h, 1.);
  s = clamp(s, 0., 1.);
  v = clamp(v, 0., 1.);
  if (v == 0.) {
    return vec3(0.);
  }
  float i = floor(h);
  float f = h - i;
  float p = v * (1. - s);
  float q = v * (1. - (s * f));
  float t = v * (1. - (s * (1. - f)));

  return i == 0 ? vec3(v, t, p) :
         i == 1 ? vec3(q, v, p) :
         i == 2 ? vec3(p, v, t) :
         i == 3 ? vec3(p, q, v) :
         i == 4 ? vec3(t, p, v) :
         i == 5 ? vec3(v, p, q) :
                  vec3(0., 0., 0.);
}

float rgb_to_hue(vec3 rgb)
{
  float c_max = max(rgb.r, max(rgb.g, rgb.b));
  float c_min = min(rgb.r, min(rgb.g, rgb.b));
  float delta = c_max - c_min;

  return mix(
      mix((rgb.r - rgb.g) / delta + 4.,
          (rgb.b - rgb.r) / delta + 2., c_max == rgb.g),
      mod((rgb.g - rgb.b) / delta, 6.), c_max == rgb.r) / 6.;
}
