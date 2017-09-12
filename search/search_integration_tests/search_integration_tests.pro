# Map library tests.

TARGET = search_integration_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..

DEPENDENCIES = generator_tests_support search_tests_support indexer_tests_support generator \
               routing routing_common search storage stats_client indexer platform editor mwm_diff \
               geometry coding base tess2 protobuf jansson succinct pugixml opening_hours icu

include($$ROOT_DIR/common.pri)

QT *= core

macx-* {
  QT *= gui widgets # needed for QApplication with event loop, to test async events (downloader, etc.)
  LIBS *= "-framework IOKit" "-framework QuartzCore" "-framework Cocoa" "-framework SystemConfiguration"
}
win32*|linux* {
  QT *= network
}

SOURCES += \
    ../../testing/testingmain.cpp \
    downloader_search_test.cpp \
    generate_tests.cpp \
    helpers.cpp \
    interactive_search_test.cpp \
    pre_ranker_test.cpp \
    processor_test.cpp \
    search_edited_features_test.cpp \
    smoke_test.cpp \

HEADERS += \
    helpers.hpp \
