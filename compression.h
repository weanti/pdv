#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <vector>

struct image_data {
  int components{};
  int width{};
  int height{};
  int bpp{};

  std::vector<int> component0;
  std::vector<int> component1;
  std::vector<int> component2;
};

image_data decompressOpenJPEG(const std::vector<char> &buf);

#endif // COMPRESSION_H
