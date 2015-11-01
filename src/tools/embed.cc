#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
  if (argc != 3) {
    std::cerr << "usage: " << argv[0] << " input output\n";
    return 1;
  }
  std::ifstream input(argv[1]);
  std::string contents;
  input.seekg(0, std::ios::end);   
  contents.reserve((std::size_t)input.tellg());
  input.seekg(0, std::ios::beg);
  contents.assign((std::istreambuf_iterator<char>(input)),
                   std::istreambuf_iterator<char>());

  std::string name = argv[1];
  for (std::size_t i = 0; i < name.size(); ++i) {
    if (!(name[i] >= 'a' && name[i] <= 'z') &&
        !(name[i] >= 'A' && name[i] <= 'Z') &&
        !(name[i] >= '0' && name[i] <= '9')) {
      name[i] = '_';
    }
  }

  std::ofstream output(argv[2]);
  output << "static const char " << name << "[] = {";
  for (std::size_t i = 0; i < contents.size(); ++i) {
    if (i % 16 == 0) {
      output << "\n  ";
    }
    output << (std::size_t)contents[i] << ", ";
  }
  output << "};\n";
  output << "static const std::size_t " <<
      name << "_len = " << contents.length() << ";\n";
  return 0;
}