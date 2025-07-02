#include "mainwindow.h"
#include <FL/Fl.H>

int main(int, char **) {
  MainWindow w(0, 0, 800, 600);
  w.show();
  return Fl::run();
}
