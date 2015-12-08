TARGET = search_tests_support
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

SOURCES += \
    test_feature.cpp \
    test_mwm_builder.cpp \
    test_results_matching.cpp \
    test_search_engine.cpp \
    test_search_request.cpp \

HEADERS += \
    test_feature.hpp \
    test_mwm_builder.hpp \
    test_results_matching.hpp \
    test_search_engine.hpp \
    test_search_request.hpp \
