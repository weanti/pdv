#include "mainwindow.h"

#include "compression.h"
#include "dicomhelpers.h"
#include "imagehelpers.h"

#include <FL/Enumerations.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_RGB_Image.H>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <vector>

MainWindow::MainWindow(int x, int y, int w, int h, const char *l)
    : Fl_Double_Window(x, y, w, h, l), mDataSet(nullptr), mMeta(nullptr),
      mIO(nullptr), mFilehandle(nullptr) {
  begin();
  mMenu = new Fl_Menu_Bar(x, y, w, 30, "menu");
  mMenu->add(
      "&File/&Open DICOM", 0,
      [](Fl_Widget *, void *data) {
        reinterpret_cast<MainWindow *>(data)->onOpenDICOM();
      },
      this);
  mImageInfo = new Fl_Multiline_Output(x, y + 30, w, 90);
  mImageDisplay = new Fl_Box(x, y + 120, w, h - 120);
  mImageDisplay->align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
  mImageDisplay->box(FL_FLAT_BOX);
  end();
}

void MainWindow::onOpenDICOM() {
  Fl_Native_File_Chooser chooser;
  if (chooser.show() != 0)
    return;
  if (chooser.count() == 0)
    return;
  if (!read(chooser.filename(0))) {
    return;
  }

  std::string error;
  std::list<std::vector<char>> frames =
      getFrames(mDataSet, mMeta, mIO, mFilehandle, error);
  const std::string txSyntax = getString(mMeta, 0x00020010);
  std::string patientName = getString(mDataSet, 0x00100010);
  std::string seriesDescription = getString(mDataSet, 0x0008103E);
  std::string modality = getString(mDataSet, 0x00080060);
  std::string imageInfoText =
      std::string("File: ") + std::string(chooser.filename(0)) +
      std::string("\n") + std::string("Patient name: ") + patientName +
      std::string("\n") + std::string("Series description: ") +
      seriesDescription + std::string("\n") + std::string("Modality: ") +
      modality + std::string("\n") + std::string("TX Sytnax: ") + txSyntax;
  // std::string("Status: ") + (error.empty() ? "OK" : error);
  mImageInfo->value(imageInfoText.c_str());

  Fl_Image *img = nullptr;
  if (!frames.empty()) {

    if (dcm_is_encapsulated_transfer_syntax(txSyntax.c_str())) {
      img = convert(decompressOpenJPEG(frames.front()));
    } else {
      unsigned int rows = getNumber(mDataSet, 0x00280010);
      unsigned int columns = getNumber(mDataSet, 0x00280011);
      unsigned int ba = getNumber(mDataSet, 0x00280100);
      std::string pi = getString(mDataSet, 0x00280004);
      image_data image;
      image.width = columns;
      image.height = rows;
      image.bpp = ba;
      if (pi == "MONOCHROME2" || pi == "MONOCHROME1") {
        image.components = 1;
        image.component0.resize(rows * columns * (ba + 7) / 8);
      }

      switch (ba) {
      case 8: {
        const unsigned int size = columns * rows;
        char *in = frames.front().data();
        int *out = image.component0.data();
        std::copy(in, in + size, out);
      } break;
      case 16: {
        const unsigned int size = columns * rows;
        short *in = (short *)(frames.front().data());
        int *out = image.component0.data();
        std::copy(in, in + size, out);
      } break;
      }

      img = convert(image);
    }
  }

  if (img) {
    if (img->w() != mImageDisplay->w() || img->h() != mImageDisplay->h()) {
      std::unique_ptr<Fl_Image> oldImage(img);
      img = img->copy(mImageDisplay->w(), mImageDisplay->h());
    }
  }
  mImageDisplay->image(img);
  redraw();
  dcm_io_close(mIO);
}

bool MainWindow::read(const char *file) {
  DcmError *error = nullptr;
  mIO = dcm_io_create_from_file(&error, file);

  mFilehandle = dcm_filehandle_create(&error, mIO);
  if (mFilehandle == nullptr)
    return false;

  mMeta = dcm_filehandle_get_file_meta(&error, mFilehandle);
  mDataSet = dcm_filehandle_read_metadata(&error, mFilehandle, nullptr);
  if (error) {
    printf("%s\n", dcm_error_get_message(error));
    return false;
  }
  return true;
}
