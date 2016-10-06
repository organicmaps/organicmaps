TARGET = partners_api_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..

INCLUDEPATH *= $$ROOT_DIR/3party/jansson/src

DEPENDENCIES = partners_api platform coding base tomcrypt jansson stats_client

include($$ROOT_DIR/common.pri)

DEFINES *= OMIM_UNIT_TEST_WITH_QT_EVENT_LOOP

QT *= core

macx-* {
  QT *= widgets # needed for QApplication with event loop, to test async events
  LIBS *= "-framework IOKit" "-framework SystemConfiguration"
}

SOURCES += \
    $$ROOT_DIR/testing/testingmain.cpp \
    booking_tests.cpp \
    uber_tests.cpp \
