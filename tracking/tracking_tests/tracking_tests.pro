TARGET = tracking_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..

INCLUDEPATH *= $$ROOT_DIR/3party/jansson/src

DEPENDENCIES = base coding geometry platform routing stats_client tracking

include($$ROOT_DIR/common.pri)

DEFINES *= OMIM_UNIT_TEST_WITH_QT_EVENT_LOOP

QT *= core

macx-* {
  QT *= widgets # needed for QApplication with event loop, to test async events
  LIBS *= "-framework IOKit" "-framework SystemConfiguration"
}

win*|linux* {
  QT *= network
}

SOURCES += \
    $$ROOT_DIR/testing/testingmain.cpp \
    reporter_tests.cpp \
