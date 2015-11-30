TARGET = platform_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = platform_tests_support platform coding base minizip tomcrypt jansson

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/jansson/src

DEFINES *= OMIM_UNIT_TEST_WITH_QT_EVENT_LOOP

QT *= core

macx-* {
  QT *= gui widgets # needed for QApplication with event loop, to test async events (downloader, etc.)
  LIBS *= "-framework IOKit" "-framework QuartzCore"
}
win32*|linux* {
  QT *= network
}

SOURCES += \
    ../../testing/testingmain.cpp \
    apk_test.cpp \
    country_file_tests.cpp \
    get_text_by_id_tests.cpp \
    jansson_test.cpp \
    language_test.cpp \
    local_country_file_tests.cpp \
    location_test.cpp \
    measurement_tests.cpp \
    platform_test.cpp \
