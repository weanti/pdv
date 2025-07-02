# pdv
Plain Dicom Viewer

This is a very simple dicom viewer. I use it for testing purposes. It may not comply to the DICOM standard in many ways.

## Dependencies:
* fltk 1.3.* (and maybe 1.4)
* libdicom 1.2+. (Some bugs are present in 1.2, but hopefully the pending pr-s are going to be accepted.)
* openjpeg 2.5.0+

## Build
It does not use pkg-config and cmake's find_package, you have to define include and library directories manually.
* FLTK_INCLUDE_DIR
* FLTK_LIBRARY_DIR
* LIBDICOM_INCLUDE_DIR
* LIBDICOM_LIBRARY_DIR
* OPENJPEG_INCLUDE_DIR
* OPENJPEG_LIBRARY_DIR

Example:
cmake -DFLTK_INCLUDE_DIR=/usr/include -DFLTK_LIBRARY_DIR=/usr/lib -DLIBDICOM_INCLUDE_DIR=/home/weanti/local/include -DLIBDICOM_LIBRARY_DIR=/home/weanti/local/lib -DOPENJPEG_INCLUDE_DIR=/usr/include -DOPENJPEG_LIBRARY_DIR=/usr/lib64 -B build/release -DCMAKE_BUILD_TYPE=Release

If you have these installed on the same location (systemwide), then it is quite redundant. It may or may not change in the future.

If you have different versions of the same library installed, then you may get compile errors. e.g. if there is fltk 1.3 installed in FLTK_LIBRARY_DIR=/usr/lib and you have fltk 1.4 in stalled in /home/myuser/local/lib and LIBDICOM_LIBRARY_DIR points there, then you may get undefined reference issues and whatnot.

It is only tested on Linux.

## Supported "formats", features
* I have tested with CT, MR, CR, XA, SC, NM, US. I managed to display them with explicit VR transfer syntaxes and also encapsulated (JPEG2000) transfer sytnaxes. Some of these require pending libdicom pr-s to be accepted.
* Only the first frame is displayed. This may change in the future.
* A bit of histogram equalization is applied for better visuals. The resolution is hardcoded at the moment.
* Photometric interpretation is not really considered.  
