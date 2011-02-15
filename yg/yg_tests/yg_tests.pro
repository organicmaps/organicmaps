TARGET = yg_tests
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = qt_tstfrm map yg indexer geometry platform coding base sgitess freetype fribidi expat

include($$ROOT_DIR/common.pri)

QT += opengl gui core

win32-g++ {
  LIBS += -lpthread
}

win32 {
  LIBS += -lopengl32 -lshell32
}

SOURCES += \
    ../../testing/testingmain.cpp \
    texture_test.cpp \
    resourcemanager_test.cpp \
    skin_loader_test.cpp \
    skin_test.cpp \
    screengl_test.cpp \
#    formats_loading_test.cpp \
    thread_render.cpp \
    opengl_test.cpp \
    screenglglobal_test.cpp \
    glyph_cache_test.cpp
