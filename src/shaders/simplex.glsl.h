ivec4 permute(ivec4 x)
{
  return (x * (1 + x * 34)) % 289;
}

ivec3 and3(bvec3 a, bvec3 b)
{
  return min(ivec3(a), ivec3(b));
}

ivec3 or3(bvec3 a, bvec3 b)
{
  return max(ivec3(a), ivec3(b));
}

float interpolate(vec3 x0, vec3 x1, vec3 x2, vec3 x3,
                  vec3 g0, vec3 g1, vec3 g2, vec3 g3)
{
  vec4 m = max(
      .6 - vec4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.);
  return 42. * dot(m * m * m * m,
      vec4(dot(g0, x0), dot(g1, x1), dot(g2, x2), dot(g3, x3)));
}

// Get the permutation: four "random" numbers in the range [0, 289).
ivec4 random4_289(ivec3 i0, ivec3 i1, ivec3 i2)
{
  i0 = ivec3(mod(i0, 289.));
  return
      permute(i0.x + ivec4(0, i1.x, i2.x, 1) +
      permute(i0.y + ivec4(0, i1.y, i2.y, 1) +
      permute(i0.z + ivec4(0, i1.z, i2.z, 1))));
}

float simplex3(vec3 coord)
{
  // Find index and corners in the simplex grid.
  ivec3 index = ivec3(floor(coord + dot(coord, vec3(1. / 3.))));
  vec3 x0 = coord - index + dot(index, vec3(1. / 6.));

  ivec3 i1 = and3(greaterThanEqual(x0, x0.yzx), greaterThan(x0, x0.zxy));
  ivec3 i2 = or3(greaterThanEqual(x0, x0.yzx), greaterThan(x0, x0.zxy));

  vec3 x1 = x0 + 1. / 6. - i1;
  vec3 x2 = x0 + 1. / 3. - i2;
  vec3 x3 = x0 - .5;

  // Gradients to choose from are 7x7 points over a square mapped onto an
  // octahedron. (Note 289 is close to a multiple of 49.)
  ivec4 gradient_index = random4_289(index, i1, i2) % 49;

  vec4 x = (2. / 7.) * (gradient_index / 7 - 1);
  vec4 y = (2. / 7.) * (gradient_index % 7 - 1);
  vec4 h = 1. - abs(x) - abs(y);

  vec4 b0 = vec4(x.xy, y.xy);
  vec4 b1 = vec4(x.zw, y.zw);

  vec4 s0 = 2. * floor(b0) + 1.;
  vec4 s1 = 2. * floor(b1) + 1.;
  vec4 sh = -step(h, vec4(0.));

  vec4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
  vec4 a1 = b1.xzyw + s1.xzyw * sh.zzww;

  vec3 g0 = normalize(vec3(a0.xy, h.x));
  vec3 g1 = normalize(vec3(a0.zw, h.y));
  vec3 g2 = normalize(vec3(a1.xy, h.z));
  vec3 g3 = normalize(vec3(a1.zw, h.w));

  return interpolate(x0, x1, x2, x3, g0, g1, g2, g3);
}
