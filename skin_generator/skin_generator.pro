TARGET = skin_generator
TEMPLATE = app
CONFIG += console

!CONFIG(map_designer_standalone) {
  CONFIG -= app_bundle
}

ROOT_DIR = ..
DEPENDENCIES = coding geometry freetype gflags base

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

HEADERS += generator.hpp
           
SOURCES += main.cpp \
           generator.cpp
