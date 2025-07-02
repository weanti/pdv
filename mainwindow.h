#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <FL/Fl_Double_Window.H>

extern "C" {
#include <dicom/dicom.h>
}

class Fl_Box;
class Fl_Menu_Bar;
class Fl_Multiline_Output;

class MainWindow : public Fl_Double_Window {

public:
  MainWindow(int x, int y, int w, int h, const char *l = nullptr);
  MainWindow(const MainWindow &) = delete;
  MainWindow &operator=(const MainWindow &) = delete;
  MainWindow(MainWindow &&) = delete;
  MainWindow &operator=(MainWindow &&) = delete;

  ~MainWindow() override = default;

public:
  void onOpenDICOM();

private:
  bool read(const char *file);

private:
  Fl_Menu_Bar *mMenu;
  Fl_Box *mImageDisplay;
  Fl_Multiline_Output* mImageInfo;
  DcmDataSet *mDataSet;
  const DcmDataSet *mMeta;
  DcmIO *mIO;
  DcmFilehandle *mFilehandle;
};
#endif // MAINWINDOW_H
