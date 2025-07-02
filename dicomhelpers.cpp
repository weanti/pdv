#include "dicomhelpers.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>
#include <string>
#include <vector>

#include "compression.h"
#include "imagehelpers.h"

#define DCM_ITEM 0xFFFEE000
#define DCM_ITEM_DELIM 0XFFFEE00D
#define DCM_SQ_DELIM 0xFFFEE0DD
#define UNDEFINED_LENGTH 0xFFFFFFFF

char *readVR(DcmIO *io) {
  static char vr[3];
  vr[2] = '\0';
  DcmError *error = nullptr;
  if (2 != dcm_io_read(&error, io, (char *)vr, 2) ||
      DCM_VR_ERROR == dcm_dict_vr_from_str(vr)) {
    if (error)
      printf("%s\n", dcm_error_get_message(error));
    return nullptr;
  }
  return vr;
}

uint32_t readTag(DcmIO *io) {
  uint16_t group = 0;
  uint16_t elem = 0;

  DcmError *error = nullptr;
  if (2 != dcm_io_read(&error, io, (char *)&group, 2)) {
    printf("%s\n", dcm_error_get_message(error));
    return 0;
  }
  if (2 != dcm_io_read(&error, io, (char *)&elem, 2)) {
    printf("%s\n", dcm_error_get_message(error));
    return 0;
  }
  const uint32_t tag = (group << 16) | elem;
  return tag;
}

std::string getString(const DcmDataSet *dataset, uint32_t tag) {
  DcmError *error = nullptr;
  DcmElement *element = dcm_dataset_get(&error, dataset, tag);
  if (!element)
    return {};
  const char *value = nullptr;
  if (!dcm_element_get_value_string(&error, element, 0, &value)) {
    printf("%s\n", dcm_error_get_message(error));
    return {};
  }
  return std::string(value);
}

int64_t getNumber(const DcmDataSet *dataset, uint32_t tag) {
  DcmError *error = nullptr;
  DcmElement *element = dcm_dataset_get(&error, dataset, tag);
  int64_t value = 0;
  if (!element) {
    printf("%s\n", dcm_error_get_message(error));
    return -1;
  }
  if (!dcm_element_get_value_integer(&error, element, 0, &value)) {
    printf("%s\n", dcm_error_get_message(error));
    return -1;
  }
  return value;
}

uint32_t readLength(DcmIO *io, uint32_t tag) {
  DcmError *error = nullptr;
  switch (tag) {
  case DCM_ITEM:
  case DCM_ITEM_DELIM:
  case DCM_SQ_DELIM: {
    uint32_t length = 0;
    if (4 != dcm_io_read(&error, io, (char *)&length, 4)) {
      printf("%s\n", dcm_error_get_message(error));
      return -1;
    }
    return length;
  }
  default:
    break;
  }

  DcmVR vr = dcm_vr_from_tag(tag);
  switch (vr) {
  case DCM_VR_AE:
  case DCM_VR_AS:
  case DCM_VR_AT:
  case DCM_VR_CS:
  case DCM_VR_DA:
  case DCM_VR_DS:
  case DCM_VR_DT:
  case DCM_VR_FL:
  case DCM_VR_FD:
  case DCM_VR_IS:
  case DCM_VR_LO:
  case DCM_VR_LT:
  case DCM_VR_PN:
  case DCM_VR_SH:
  case DCM_VR_SL:
  case DCM_VR_SS:
  case DCM_VR_ST:
  case DCM_VR_TM:
  case DCM_VR_UI:
  case DCM_VR_UL:
  case DCM_VR_US: {
    uint16_t length = 0;
    if (2 != dcm_io_read(&error, io, (char *)&length, 2)) {
      printf("%s\n", dcm_error_get_message(error));
      return -1;
    }
    return length;
  }
  default: {
    uint16_t reserved = 0;
    if (2 != dcm_io_read(&error, io, (char *)&reserved, 2)) {
      printf("%s\n", dcm_error_get_message(error));
      return -1;
    }
    if (reserved != 0) {
      return -1;
    }

    uint32_t length = 0;
    if (4 != dcm_io_read(&error, io, (char *)&length, 4)) {
      printf("%s\n", dcm_error_get_message(error));
      return -1;
    }
    return length;
  }
  }
}

DcmElement *readDataElement(DcmIO *io, uint32_t tag, bool explicitVR) {
  DcmVR vr;
  if (explicitVR) {
    vr = dcm_dict_vr_from_str(readVR(io));
  } else {
    vr = dcm_vr_from_tag(tag);
  }
  DcmError *error = nullptr;

  if (vr == DCM_VR_SQ) {
    // DICOM table 7.5-2 shows 2 bytes reserved after the VR. ????? do we need
    // to read that?
    DcmSequence *sequence = readSequence(io, tag, explicitVR);
    DcmElement *element = dcm_element_create(&error, tag, vr);
    if (dcm_element_set_value_sequence(&error, element, sequence))
      return element;
  }
  const uint32_t length = readLength(io, tag);

  DcmElement *element = dcm_element_create(&error, tag, vr);
  switch (vr) {
  case DCM_VR_AE:
  case DCM_VR_AS:
  case DCM_VR_CS:
  case DCM_VR_DA:
  case DCM_VR_DS:
  case DCM_VR_DT:
  case DCM_VR_IS:
  case DCM_VR_LO:
  case DCM_VR_LT:
  case DCM_VR_PN:
  case DCM_VR_SH:
  case DCM_VR_ST:
  case DCM_VR_TM:
  case DCM_VR_UC:
  case DCM_VR_UI:
  case DCM_VR_UR:
  case DCM_VR_UT: {
    char *buf = (char *)malloc(length + 1);
    dcm_io_read(&error, io, buf, length);
    dcm_element_set_value_string(&error, element, buf, false);
    free(buf);
  } break;
  case DCM_VR_AT: {
    int32_t value = 0;
    dcm_io_read(&error, io, (char *)&value, length);
    dcm_element_set_value_integer(&error, element, value);
  } break;
  case DCM_VR_FL: {
    float value = 0;
    dcm_io_read(&error, io, (char *)&value, length);
    dcm_element_set_value_decimal(&error, element, value);
  } break;
  case DCM_VR_FD: {
    double value = 0;
    dcm_io_read(&error, io, (char *)&value, length);
    dcm_element_set_value_decimal(&error, element, value);
  } break;
  case DCM_VR_OB:
  case DCM_VR_OD:
  case DCM_VR_OF:
  case DCM_VR_OL:
  case DCM_VR_OV:
  case DCM_VR_OW: {
    if (length != UNDEFINED_LENGTH) {
      char *buf = (char *)malloc(length + 1);
      dcm_io_read(&error, io, buf, length);
      dcm_element_set_value_binary(&error, element, buf, length, false);
      free(buf);
    } else {
      uint32_t next_tag = readTag(io);
      if (next_tag == DCM_ITEM) { // item, Basic Offset Table
        uint32_t bot_length = readLength(io, next_tag);
        if (bot_length == 0) // 1 fragment = 1 frame OR all fragments = 1 frame
        {
          std::vector<char> pixeldata;
          next_tag = readTag(io);
          while (next_tag != DCM_SQ_DELIM) {
            if (next_tag == DCM_ITEM) {
              uint32_t next_length = readLength(io, next_tag);
              uint32_t original_size = pixeldata.size();
              pixeldata.resize(original_size + next_length);
              dcm_io_read(&error, io, pixeldata.data() + original_size,
                          next_length);
            }
            next_tag = readTag(io);
          }
          if (!pixeldata.empty()) {
            dcm_element_set_value_binary(&error, element, pixeldata.data(),
                                         pixeldata.size(), false);
          }
        }
      }
    }
  } break;
  case DCM_VR_SL: {
    int32_t value = 0;
    dcm_io_read(&error, io, (char *)&value, length);
    dcm_element_set_value_integer(&error, element, value);
  } break;
  case DCM_VR_SS: {
    int16_t value = 0;
    dcm_io_read(&error, io, (char *)&value, length);
    dcm_element_set_value_integer(&error, element, value);
  } break;
  case DCM_VR_SV: {
    int64_t value = 0;
    dcm_io_read(&error, io, (char *)&value, length);
    dcm_element_set_value_integer(&error, element, value);
  } break;
  case DCM_VR_UL: {
    uint32_t value = 0;
    dcm_io_read(&error, io, (char *)&value, length);
    dcm_element_set_value_integer(&error, element, value);
  } break;
  case DCM_VR_US: {
    uint16_t value = 0;
    dcm_io_read(&error, io, (char *)&value, length);
    dcm_element_set_value_integer(&error, element, value);
  } break;
  case DCM_VR_UV: {
    uint64_t value = 0;
    dcm_io_read(&error, io, (char *)&value, length);
    dcm_element_set_value_integer(&error, element, value);
  } break;
  }

  return element;
}

DcmSequence *readSequence(DcmIO *io, uint32_t tag, bool explicitVR) {
  DcmError *error = nullptr;
  DcmSequence *seq = dcm_sequence_create(&error);
  DcmDataSet *ds = dcm_dataset_create(&error);
  uint32_t length = readLength(io, tag);
  DcmElement *element = nullptr;
  do {
    tag = readTag(io);

    if (tag == DCM_ITEM) // item tag
    {
      const uint32_t item_length = readLength(io, tag);
      tag = readTag(io);
      element = readDataElement(io, tag, explicitVR);
      if (item_length != UNDEFINED_LENGTH && length != UNDEFINED_LENGTH &&
          element)
        length -= item_length;
      dcm_dataset_insert(&error, ds, element);
      continue;
    }
    if (tag == DCM_ITEM_DELIM) // tag item delim, its length shall be 0
    {
      const uint32_t item_length = readLength(io, tag);
      if (item_length != 0)
        return nullptr;
      continue;
    }
    if (tag == DCM_SQ_DELIM) {
      const uint32_t item_length = readLength(io, tag);
      if (item_length != 0)
        return nullptr;
      break;
    }

    element = readDataElement(io, tag, explicitVR);
    const uint32_t item_length = dcm_element_get_length(element);
    if (item_length != UNDEFINED_LENGTH && length != UNDEFINED_LENGTH &&
        element)
      length -= item_length;
    dcm_dataset_insert(&error, ds, element);
  } while (length > 0);
  dcm_sequence_append(&error, seq, ds);
  return seq;
}

std::list<std::vector<char>> getFrames(DcmDataSet *dataset,
                                       const DcmDataSet *meta, DcmIO *io,
                                       DcmFilehandle *filehandle,
                                       std::string &msg) {
  DcmError *error = nullptr;
  dcm_error_clear(&error);

  if (dcm_filehandle_prepare_read_frame(&error, filehandle)) {
    int64_t framecnt = atol(getString(dataset, 0x00280008).c_str());
    if (framecnt <= 0)
      framecnt = 1;
    std::list<std::vector<char>> frames;
    for (unsigned int idx = 1; idx <= framecnt; idx++) {
      DcmFrame *frame = dcm_filehandle_read_frame(&error, filehandle, idx);
      uint32_t length = dcm_frame_get_length(frame);
      const char *value = dcm_frame_get_value(frame);
      std::vector<char> pixels(length);
      std::copy(value, value + length, pixels.data());
      frames.push_back(pixels);
    }
    return frames;
  }

  if (error) {
    printf("%s\n", dcm_error_get_message(error));
    if (dcm_error_get_code(error) == DCM_ERROR_CODE_PARSE) {
      msg = std::string(dcm_error_get_message(error));
      return {};
    }
  }
  const std::string txSyntax = getString(meta, 0x00020010);
  bool explicitVR = txSyntax != std::string("1.2.840.10008.1.2");

  uint32_t tag = readTag(io);
  DcmElement *element = readDataElement(io, tag, explicitVR);
  while (tag != 0x7FE00010) {
    tag = readTag(io);
    element = readDataElement(io, tag, explicitVR);
  }
  if (element && dcm_element_get_tag(element) == 0x7FE00010) {
    const uint32_t length = dcm_element_get_length(element);
    const char *value = nullptr;
    dcm_element_get_value_binary(&error, element, (const void **)&value);
    std::vector<char> pixels(length);
    std::copy(value, value + length, pixels.data());
    std::list<std::vector<char>> frames;
    frames.push_back(pixels);
    return frames;
  }

  return {};
}

bool print_element(const DcmElement *element, void *data) {
  FILE *fout = (FILE *)data;
  fprintf(fout, "%#0.8x %s ", dcm_element_get_tag(element),
          dcm_dict_str_from_vr(dcm_element_get_vr(element)));
  DcmError *error = nullptr;
  switch (dcm_dict_vr_class(dcm_element_get_vr(element))) {
  case DCM_VR_CLASS_NUMERIC_DECIMAL: {
    const uint32_t vm = dcm_element_get_vm(element);
    for (uint32_t i = 0; i < vm; i++) {
      double val = 0.0;
      dcm_element_get_value_decimal(&error, element, i, &val);
      fprintf(fout, "%lf ", val);
    }
  } break;
  case DCM_VR_CLASS_NUMERIC_INTEGER: {
    const uint32_t vm = dcm_element_get_vm(element);
    for (uint32_t i = 0; i < vm; i++) {
      int64_t val = 0;
      dcm_error_clear(&error);
      dcm_element_get_value_integer(&error, element, i, &val);
      fprintf(fout, "%lli ", val);
    }
  } break;
  case DCM_VR_CLASS_STRING_SINGLE: {
    const char *val = nullptr;
    dcm_error_clear(&error);
    dcm_element_get_value_string(&error, element, 0, &val);
  } break;
  case DCM_VR_CLASS_STRING_MULTI: {
    const uint32_t vm = dcm_element_get_vm(element);
    for (uint32_t i = 0; i < vm; i++) {
      const char *val = nullptr;
      dcm_error_clear(&error);
      dcm_element_get_value_string(&error, element, i, &val);
      fprintf(fout, "%s ", val);
    }
  } break;
  default:
    break;
  }
  fprintf(fout, "\n");

  return true;
}
