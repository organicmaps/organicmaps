# This subproject implements routing consistency tests.
# This tests are launched on the whole world dataset.

TARGET = routing_consistency_test
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = map routing traffic routing_common transit search storage mwm_diff indexer platform \
               editor geometry coding base osrm jansson protobuf bsdiff succinct stats_client \
               generator gflags pugixml icu agg

include($$ROOT_DIR/common.pri)

INCLUDEPATH += $$ROOT_DIR/3party/jansson/src

QT *= core

!iphone*:!android*:!tizen:!macx-* {
  QT *= network
}

macx-* {
  QT *= gui widgets # needed for QApplication with event loop, to test async events (downloader, etc.)
  LIBS *= "-framework IOKit" "-framework QuartzCore" "-framework Cocoa" "-framework SystemConfiguration"
}
win32*|linux* {
  QT *= network
}

INCLUDEPATH *= $$ROOT_DIR/3party/gflags/src

SOURCES += \
  ../routing_integration_tests/routing_test_tools.cpp \
  routing_consistency_tests.cpp \

HEADERS += \
