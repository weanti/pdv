#include "imagehelpers.h"
#include <FL/Fl_RGB_Image.H>
#include <openjpeg-2.5/openjpeg.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <vector>

Fl_Image *convert(const image_data &image) {
  if (image.components == 1) {
    if (image.bpp > 8) {
      const unsigned int size = image.width * image.height;
      const int *pdata = image.component0.data();
      // TODO: do the normalization and cast to char in 1 step?
      std::vector<uint16_t> intermediate(size);
      std::transform(pdata, pdata + (size), intermediate.data(),
                     [](int value) { return static_cast<uint16_t>(value); });
      histeq(intermediate);
      normalizeToBitsUsed(intermediate, 8);
      unsigned char *converted = (unsigned char *)malloc(size);
      std::transform(intermediate.cbegin(), intermediate.cend(), converted,
                     [](unsigned char value) {
                       return static_cast<unsigned char>(value);
                     });
      Fl_Image *copy =
          Fl_RGB_Image(converted, image.width, image.height, 1, 0).copy();
      free(converted);
      return copy;
    }
    if (image.bpp == 8) {
      std::vector<uint8_t> converted(image.component0.size());
      const int *pdata = image.component0.data();
      std::transform(pdata, pdata + image.component0.size(), converted.data(),
                     [](int value) { return static_cast<uint8_t>(value); });
      return Fl_RGB_Image(converted.data(), image.width, image.height, 1, 0)
          .copy(image.width, image.height);
    }
    if (image.bpp == 1) {
      const unsigned int size = image.width * image.height;
      unsigned char *converted = (unsigned char *)malloc(size);
      unsigned char *cur = converted;
      for (unsigned int i = 0; i < size; i++) {
        *cur = (image.component0.data()[i] != 0) ? 0x80 : 0x0;
        cur++;
      }
      Fl_Image *copy =
          Fl_RGB_Image(converted, image.width, image.height, 1, 0).copy();
      free(converted);
      return copy;
    }
  } else if (image.components == 3) {
    const int *pred = image.component0.data();
    const int *pgreen = image.component1.data();
    const int *pblue = image.component2.data();
    const unsigned int size = image.width * image.height;
    unsigned char *converted = (unsigned char *)malloc(size * 3);
    unsigned char *pdata = converted;
    for (unsigned int i = 0; i < size; i++) {
      *pdata = *pred;
      pdata++;

      *pdata = *pgreen;
      pdata++;

      *pdata = *pblue;
      pdata++;

      pred++;
      pgreen++;
      pblue++;
    }
    Fl_Image *copy =
        Fl_RGB_Image(converted, image.width, image.height, 3).copy();
    free(converted);
    return copy;
  }
  return nullptr;
}
