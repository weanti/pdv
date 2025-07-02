cmake_minimum_required(VERSION 3.16)

# pdv = plain dicom viewer
project(pdv LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    dicomhelpers.h
    dicomhelpers.cpp
    imagehelpers.h
    imagehelpers.cpp
    compression.h
    compression.cpp
)

add_executable(pdv
    ${PROJECT_SOURCES}
)

target_include_directories( pdv
    PRIVATE
        ${FLTK_INCLUDE_DIR}
        ${LIBDICOM_INCLUDE_DIR}
        ${OPENJPEG_INCLUDE_DIR}
)
target_link_directories( pdv
    PRIVATE
        ${FLTK_LIBRARY_DIR}
        ${LIBDICOM_LIBRARY_DIR}
        ${OPENJPEG_LIBRARY_DIR}
)
target_link_libraries(pdv
    PRIVATE
        fltk_images
        fltk

        # jpeg 2000 lib
        openjp2
        # DICOM lib
        dicom
)
