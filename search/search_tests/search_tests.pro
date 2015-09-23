# Map library tests.

TARGET = search_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = search indexer platform geometry coding base protobuf tomcrypt

include($$ROOT_DIR/common.pri)

QT *= core

macx-*: LIBS *= "-framework IOKit"

SOURCES += \
    ../../testing/testingmain.cpp \
    algos_tests.cpp \
    house_detector_tests.cpp \
    keyword_lang_matcher_test.cpp \
    keyword_matcher_test.cpp \
    latlon_match_test.cpp \
    locality_finder_test.cpp \
    query_saver_tests.cpp \
    string_intersection_test.cpp \
    string_match_test.cpp \

HEADERS += \
    match_cost_mock.hpp \
