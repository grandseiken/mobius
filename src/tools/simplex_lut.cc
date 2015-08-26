#include <cmath>
#include <iomanip>
#include <iostream>

int main() {
  const long gradient_texture_size = 64;
  std::cout << std::fixed << std::setprecision(16);
  std::cout << "static const float gen_simplex_gradient_lut[] = {\n";
  // 7x7 points over a square mapped onto an octahedron.
  for (int yi = 0; yi < 7; ++yi) {
    for (int xi = 0; xi < 7; ++xi) {
      // X and Y evenly distributed over [-1, 1].
      double x = (2 * xi - 6) / 7.;
      double y = (2 * yi - 6) / 7.;

      // H in [-1, 1] with H + |X| + |Y| = 1.
      double h = 1 - fabs(x) - fabs(y);

      // Swaps the lower corners of the pyramid to form the octahedron.
      double ax = x - (2 * floor(x) + 1) * (h <= 0 ? 1 : 0);
      double ay = y - (2 * floor(y) + 1) * (h <= 0 ? 1 : 0);

      // Normalize.
      double len = sqrt(ax * ax + ay * ay + h * h);
      ax = .5 + ax / (len * 2);
      ay = .5 + ay / (len * 2);
      h = .5 + h / (len * 2);

      std::cout << "  " << ax << ", " << ay << ", " << h << ",\n";
    }
  }
  for (int i = 0; i < gradient_texture_size - 49; ++i) {
    std::cout << "  0, 0, 0,\n";
  }
  std::cout << "};\n\n";

  const long permutation_texture_size = 2048;
  const long prime_factor = 43;
  const long ring_size = prime_factor * prime_factor;
  std::cout << "static const float gen_simplex_permutation_lut[] = {\n";
  for (long i = 0; i < permutation_texture_size; ++i) {
    long x = (i % ring_size);
    long out = (1 + x * (1 + 2 * x * prime_factor)) % ring_size;
    double f = double(out) / permutation_texture_size;
    std::cout << "  " << f << ",\n";
  }
  std::cout << "};\n\n";
}
