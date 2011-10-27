TARGET = platform_tests
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = platform coding base tomcrypt jansson

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/jansson/src

DEFINES *= OMIM_UNIT_TEST_WITH_QT_EVENT_LOOP

QT *= core

win32* {
  LIBS *= -lShell32
  win32-g++: LIBS *= -lpthread
}
macx* {
  QT *= gui # needed for QApplication with event loop, to test async events (downloader, etc.)
  LIBS *= "-framework Foundation" "-framework IOKit"
}
win32*|linux* {
  QT *= network
}

SOURCES += \
    ../../testing/testingmain.cpp \
    platform_test.cpp \
    download_test.cpp \
    jansson_test.cpp \
    concurrent_runner_test.cpp \
    language_test.cpp \
    url_generator_test.cpp \
