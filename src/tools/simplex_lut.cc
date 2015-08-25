#include <cmath>
#include <iomanip>
#include <iostream>

int main() {
  std::cout << std::fixed << std::setprecision(16);
  std::cout << "static const float gen_simplex_lut[] = {\n";
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
  for (int i = 0; i < 64 - 49; ++i) {
    std::cout << "  0, 0, 0,\n";
  }
  std::cout << "};\n";
}
