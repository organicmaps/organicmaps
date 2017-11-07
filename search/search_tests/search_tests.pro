# Map library tests.

TARGET = search_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = search_tests_support search indexer platform editor geometry coding icu base protobuf jansson succinct pugixml stats_client

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
    ../../testing/testingmain.cpp \
    algos_tests.cpp \
    highlighting_tests.cpp \
    hotels_filter_test.cpp \
    house_detector_tests.cpp \
    house_numbers_matcher_test.cpp \
    interval_set_test.cpp \
    keyword_lang_matcher_test.cpp \
    keyword_matcher_test.cpp \
    latlon_match_test.cpp \
    locality_finder_test.cpp \
    locality_scorer_test.cpp \
    locality_selector_test.cpp \
    point_rect_matcher_tests.cpp \
    query_saver_tests.cpp \
    ranking_tests.cpp \
    segment_tree_tests.cpp \
    string_match_test.cpp \

HEADERS += \
    match_cost_mock.hpp \
