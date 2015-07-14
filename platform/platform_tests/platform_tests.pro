TARGET = platform_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = platform coding base zlib tomcrypt jansson

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/jansson/src

DEFINES *= OMIM_UNIT_TEST_WITH_QT_EVENT_LOOP

QT *= core

win32* {
  LIBS *= -lShell32
  win32-g++: LIBS *= -lpthread
}
macx-* {
  QT *= gui widgets # needed for QApplication with event loop, to test async events (downloader, etc.)
  LIBS *= "-framework Foundation" "-framework IOKit" "-framework QuartzCore"
}
win32*|linux* {
  QT *= network
}

SOURCES += \
    ../../testing/testingmain.cpp \
    apk_test.cpp \
    country_file_tests.cpp \
    downloader_test.cpp \
    jansson_test.cpp \
    language_test.cpp \
    local_country_file_tests.cpp \
    location_test.cpp \
    measurement_tests.cpp \
    platform_test.cpp \
    video_timer_test.cpp \

HEADERS += \
    file_utils.hpp \
