TARGET = skin_generator
TEMPLATE = app
CONFIG   += console

!map_designer {
  CONFIG   -= app_bundle
}

ROOT_DIR = ..
include($$ROOT_DIR/common.pri)

QT *= core gui svg xml

PRE_TARGETDEPS += $$BINARIES_PATH/$${LIB_PREFIX}coding$$LIB_EXT
PRE_TARGETDEPS += $$BINARIES_PATH/$${LIB_PREFIX}geometry$$LIB_EXT
PRE_TARGETDEPS += $$BINARIES_PATH/$${LIB_PREFIX}freetype$$LIB_EXT
PRE_TARGETDEPS += $$BINARIES_PATH/$${LIB_PREFIX}gflags$$LIB_EXT
PRE_TARGETDEPS += $$BINARIES_PATH/$${LIB_PREFIX}base$$LIB_EXT

LIBS += -lcoding -lgeometry -lfreetype -lgflags -lbase

INCLUDEPATH += $$ROOT_DIR/3party/boost \
               $$ROOT_DIR/3party/freetype/include \
               $$ROOT_DIR/3party/gflags/src

HEADERS += skin_generator.hpp
           
SOURCES += main.cpp \
    skin_generator.cpp
