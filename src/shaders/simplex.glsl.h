// Adapted from: https://github.com/ashima/webgl-noise
// which has the following license:
//
// Copyright (C) 2011 by Ashima Arts (Simplex noise)
// Copyright (C) 2011 by Stefan Gustavson (Classic noise)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// Must be small enough that permute(x) can be calculated without overflowing.
// We can use something higher with integers, but it's slow.
const int gradient_size = 49;
const int permutation_prime_factor = 17;
const int permutation_ring_size =
    permutation_prime_factor * permutation_prime_factor;
const int lut_permutation_ring_size = 17 * 17;

vec4 permute(sampler1D lut, bool use_lut, vec4 x)
{
  if (use_lut) {
    // It should now be possible to turn up the permutation size, but the hash
    // breaks down at e.g. 43 * 43 in a texture of size 2048. Work out why.
    vec4 s = (.5 + x) / 512.;
    return 512. * vec4(
        texture(lut, s.x).x, texture(lut, s.y).x,
        texture(lut, s.z).x, texture(lut, s.w).x);
  }
  return mod(
      1 + x * (1 + 2 * x * permutation_prime_factor), permutation_ring_size);
}

// Get the permutation: four "random" numbers.
vec4 random4(sampler1D lut, bool use_lut, vec3 i0, vec3 i1, vec3 i2)
{
  i0 = mod(i0, use_lut ? lut_permutation_ring_size : permutation_ring_size);
  return
      permute(lut, use_lut, i0.x + vec4(0, i1.x, i2.x, 1) +
      permute(lut, use_lut, i0.y + vec4(0, i1.y, i2.y, 1) +
      permute(lut, use_lut, i0.z + vec4(0, i1.z, i2.z, 1))));
}

float interpolate(vec3 x0, vec3 x1, vec3 x2, vec3 x3,
                  vec3 g0, vec3 g1, vec3 g2, vec3 g3)
{
  vec4 m = max(
      .6 - vec4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.);
  return 42. * dot(m * m * m * m,
      vec4(dot(g0, x0), dot(g1, x1), dot(g2, x2), dot(g3, x3)));
}

float simplex3(vec3 coord, sampler1D gradient_lut,
               bool use_permutation_lut, sampler1D permutation_lut)
{
  // Find index and corners in the simplex grid.
  vec3 index = floor(coord + dot(coord, vec3(1. / 3.)));
  vec3 x0 = coord - index + dot(index, vec3(1. / 6.));

  vec3 order = step(x0.yzx, x0.xyz);
  vec3 order_inv = 1.0 - order;
  vec3 i1 = min(order.xyz, order_inv.zxy);
  vec3 i2 = max(order.xyz, order_inv.zxy);

  vec3 x1 = x0 + 1. / 6. - i1;
  vec3 x2 = x0 + 1. / 3. - i2;
  vec3 x3 = x0 - .5;

  // Gradients to choose from are 7x7 points over a square mapped onto an
  // octahedron.
  vec4 r = random4(permutation_lut, use_permutation_lut, index, i1, i2);
  vec4 gradient_index = mod(r, gradient_size);
  vec4 tex_coords = (.5 + gradient_index) / 64.;

  vec3 g0 = 2. * texture(gradient_lut, tex_coords.x).xyz - 1.;
  vec3 g1 = 2. * texture(gradient_lut, tex_coords.y).xyz - 1.;
  vec3 g2 = 2. * texture(gradient_lut, tex_coords.z).xyz - 1.;
  vec3 g3 = 2. * texture(gradient_lut, tex_coords.w).xyz - 1.;

  return interpolate(x0, x1, x2, x3, g0, g1, g2, g3);
}
