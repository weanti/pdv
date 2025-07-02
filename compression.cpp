#include "compression.h"

#include <openjpeg-2.5/openjpeg.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

struct read_pointer {
  const char *buf;
  size_t size;
  size_t cur;
};

opj_stream_t *setup_stream(read_pointer &state);
opj_codec_t *setup_codec(OPJ_CODEC_FORMAT format, opj_dparameters_t &params);

void msg(const char *msg, void *client_data);
OPJ_SIZE_T read(void *p_buffer, OPJ_SIZE_T p_nb_bytes, void *p_user_data);
OPJ_BOOL seek(OPJ_OFF_T p_nb_bytes, void *p_user_data);
OPJ_OFF_T skip(OPJ_OFF_T p_nb_bytes, void *p_user_data);

void dump_img(opj_image_t *image, const char *fname);
void dump_tile(OPJ_BYTE *data, OPJ_UINT32 size, const char *fname);

opj_stream_t *setup_stream(read_pointer &state) {
  opj_stream_t *stream =
      opj_stream_create(state.size, /*input stream*/ OPJ_TRUE);
  opj_stream_set_read_function(stream, read);
  opj_stream_set_seek_function(stream, seek);
  opj_stream_set_skip_function(stream, skip);

  opj_stream_set_user_data(stream, &state, nullptr);
  opj_stream_set_user_data_length(stream, state.size);
  return stream;
}

opj_codec_t *setup_codec(OPJ_CODEC_FORMAT format, opj_dparameters_t &params) {
  memset(&params, 0, sizeof(opj_dparameters_t));
  opj_set_default_decoder_parameters(&params);
  opj_codec_t *codec = opj_create_decompress(format);
  if (!codec) {
    fprintf(stderr, "codec failure\n");
    return nullptr;
  }
  opj_set_error_handler(codec, msg, nullptr);
  opj_set_warning_handler(codec, msg, nullptr);
  if (opj_setup_decoder(codec, &params) != OPJ_TRUE) {
    fprintf(stderr, "decoder failure\n");
    return nullptr;
  }
  if (opj_codec_set_threads(codec, 1) != OPJ_TRUE) {
    fprintf(stderr, "thread failure\n");
    return nullptr;
  }
  return codec;
}

void msg(const char *msg, void * /*unused*/) { fprintf(stderr, "%s \n", msg); }

OPJ_SIZE_T read(void *p_buffer, OPJ_SIZE_T p_nb_bytes, void *p_user_data) {
  fprintf(stderr, "requesting %lu bytes\n", p_nb_bytes);

  read_pointer *state = reinterpret_cast<read_pointer *>(p_user_data);
  p_nb_bytes =
      std::min(p_nb_bytes, static_cast<OPJ_SIZE_T>(state->size - state->cur));
  if (p_nb_bytes > 0) {
    fprintf(stderr, "delivering %lu bytes\n", p_nb_bytes);
    std::copy(state->buf + state->cur, state->buf + state->cur + p_nb_bytes,
              reinterpret_cast<unsigned char *>(p_buffer));
    state->cur = state->cur + p_nb_bytes;
    return p_nb_bytes;
  }
  return -1;
}

OPJ_BOOL seek(OPJ_OFF_T p_nb_bytes, void *p_user_data) {
  fprintf(stderr, "seek to %li\n", p_nb_bytes);
  read_pointer *state = reinterpret_cast<read_pointer *>(p_user_data);
  if ((0 <= p_nb_bytes) && (p_nb_bytes < state->size)) {
    state->cur = p_nb_bytes;
    return OPJ_TRUE;
  }
  return OPJ_FALSE;
}

OPJ_OFF_T skip(OPJ_OFF_T p_nb_bytes, void *p_user_data) {
  fprintf(stderr, "skip %li\n", p_nb_bytes);
  read_pointer *state = reinterpret_cast<read_pointer *>(p_user_data);
  if ((0 <= state->cur + p_nb_bytes) &&
      (state->cur + p_nb_bytes < state->size)) {
    state->cur += p_nb_bytes;
    return OPJ_TRUE;
  }
  return OPJ_FALSE;
}

void dump_img(opj_image_t *image, const char *fname) {
  FILE *fout = fopen(fname, "w");
  const size_t size = image->comps[0].w * image->comps[0].h;
  if (fwrite(image->comps[0].data, (image->comps[0].prec + 7) / 8, size,
             fout) == 0) {
    fprintf(stderr, "failed to write %d\n", ferror(fout));
  }
  fclose(fout);
}

void dump_tile(OPJ_BYTE *data, OPJ_UINT32 size, const char *fname) {
  FILE *fout = fopen(fname, "w");
  if (fwrite(data, 1, size, fout) == 0) {
    fprintf(stderr, "failed to write %d\n", ferror(fout));
  }
  fclose(fout);
}

image_data decompressOpenJPEG(const std::vector<char> &buf) {

  read_pointer state;
  state.buf = buf.data();
  state.cur = 0;
  state.size = buf.size();
  opj_stream_t *stream = setup_stream(state);
  opj_dparameters_t params;
  opj_codec_t *codec = setup_codec(OPJ_CODEC_J2K, params);
  if (!codec) {
    return {};
  }
  opj_image_t *image = nullptr;
  if (opj_read_header(stream, codec, &image) != OPJ_TRUE) {
    fprintf(stderr, "read failure with current codec\n");
    opj_stream_destroy(stream);
    opj_destroy_codec(codec);
    stream = nullptr;
    codec = nullptr;
    image = nullptr;
    state.buf = buf.data();
    state.cur = 0;
    state.size = buf.size();
    stream = setup_stream(state);
    codec = setup_codec(OPJ_CODEC_JP2, params);
    if (opj_read_header(stream, codec, &image) != OPJ_TRUE) {
      fprintf(stderr, "read failure with current codec, no more tries\n");
      return {};
    }
  }
  if (opj_set_decode_area(codec, image, 0, 0, 0, 0) != OPJ_TRUE) {
    fprintf(stderr, "area failure\n");
    return {};
  }
  if (opj_decode(codec, stream, image) != OPJ_TRUE) {
    fprintf(stderr, "decode failure\n");
    return {};
  }

  fprintf(stderr, "comps %d, color %d\n", image->numcomps, image->color_space);
  if (opj_end_decompress(codec, stream) != OPJ_TRUE) {
    fprintf(stderr, "end failure\n");
    return {};
  }
  image_data img;

  switch (image->numcomps) {
  case 1: {
    const unsigned int size = image->comps[0].w * image->comps[0].h;
    img.width = image->comps[0].w;
    img.height = image->comps[0].h;
    img.bpp = image->comps[0].prec;
    img.component0.resize(size);
    std::copy(image->comps[0].data, image->comps[0].data + size,
              img.component0.data());
  } break;
  case 3: {
    int size = image->comps[0].w * image->comps[0].h;
    img.width = image->comps[0].w;
    img.height = image->comps[0].h;
    img.bpp = image->comps[0].prec;
    img.component0.resize(size);
    img.component1.resize(size);
    img.component2.resize(size);
    std::copy(image->comps[0].data, image->comps[0].data + size,
              img.component0.data());
    std::copy(image->comps[1].data, image->comps[1].data + size,
              img.component1.data());
    std::copy(image->comps[2].data, image->comps[2].data + size,
              img.component2.data());
  } break;
  default:
    return {};
  }
  img.components = image->numcomps;
  opj_stream_destroy(stream);
  opj_destroy_codec(codec);
  opj_image_destroy(image);
  return img;
}
