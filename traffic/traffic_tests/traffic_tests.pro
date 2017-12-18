TARGET = traffic_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..

DEPENDENCIES = traffic routing_common indexer platform_tests_support platform coding geometry base stats_client protobuf icu

include($$ROOT_DIR/common.pri)

DEFINES *= OMIM_UNIT_TEST_WITH_QT_EVENT_LOOP
INCLUDEPATH *= $$ROOT_DIR/3party/jansson/src

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
    traffic_info_test.cpp \
