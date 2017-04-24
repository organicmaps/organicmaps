# Search quality library tests.

TARGET = search_quality_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../../..
# todo(@m) revise
DEPENDENCIES = map drape_frontend routing traffic routing_common search_tests_support search_quality search storage indexer drape platform geometry coding base \
               freetype expat gflags jansson protobuf osrm stats_client minizip succinct \
               opening_hours stb_image sdf_image icu

include($$ROOT_DIR/common.pri)

INCLUDEPATH += $$ROOT_DIR/3party/jansson/src

QT *= core

macx-* {
  QT *= gui widgets # needed for QApplication with event loop, to test async events (downloader, etc.)
  LIBS *= "-framework IOKit" "-framework QuartzCore" "-framework Cocoa" "-framework SystemConfiguration"
}
win32*|linux* {
  QT *= network
}

SOURCES += \
    ../../../testing/testingmain.cpp \
    sample_test.cpp \

HEADERS += \
