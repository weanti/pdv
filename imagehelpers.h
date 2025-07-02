#ifndef IMAGEHELPERS_H
#define IMAGEHELPERS_H

#include "compression.h"

// for windows
#undef max

#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

class Fl_Image;

template <typename T> void histeq(std::vector<T> &buf) {
  const T maxval = std::numeric_limits<T>::max();
  std::vector<uint32_t> hist(maxval);
  std::vector<uint32_t> new_val(maxval);
  for (auto &val : buf) {
    hist[val]++;
  }
  uint64_t c = 0;
  for (auto i = 0; i < maxval; i++) {
    c += hist[i];
    new_val[i] = std::ceil(static_cast<double>(c) * maxval / buf.size());
  }
  for (auto i = 0; i < buf.size(); i++) {
    buf[i] = new_val[buf[i]];
  }
}

template <typename T>
void normalizeToBitsUsed(std::vector<T> &buf, unsigned int bits) {
  const double factor =
      static_cast<double>(std::numeric_limits<T>::max()) / (1 << bits);
  for (size_t i = 0; i < buf.size(); ++i) {
    buf[i] = buf[i] * factor;
  }
}

Fl_Image *convert(const image_data &image);

#endif // IMAGEHELPERS_H
