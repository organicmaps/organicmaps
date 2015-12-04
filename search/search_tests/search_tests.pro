# Map library tests.

TARGET = search_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = search indexer platform geometry coding base protobuf tomcrypt succinct

include($$ROOT_DIR/common.pri)

QT *= core

macx-*: LIBS *= "-framework IOKit"

SOURCES += \
    ../../testing/testingmain.cpp \
    algos_tests.cpp \
    categories_test.cpp \
    house_detector_tests.cpp \
    house_numbers_matcher_test.cpp \
    interval_set_test.cpp \
    keyword_lang_matcher_test.cpp \
    keyword_matcher_test.cpp \
    latlon_match_test.cpp \
    locality_finder_test.cpp \
    query_saver_tests.cpp \
    search_string_utils_test.cpp \
    string_intersection_test.cpp \
    string_match_test.cpp \

HEADERS += \
    match_cost_mock.hpp \
