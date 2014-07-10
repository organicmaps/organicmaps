# Search library.

TARGET = search
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

HEADERS += \
    search_common.hpp \
    search_engine.hpp \
    intermediate_result.hpp \
    keyword_matcher.hpp \
    search_query.hpp \
    result.hpp \
    latlon_match.hpp \
    approximate_string_match.hpp \
    feature_offset_match.hpp \
    keyword_lang_matcher.hpp \
    params.hpp \
    house_detector.hpp \
    algos.hpp \
    indexed_value.hpp \
    geometry_utils.hpp \
    search_string_intersection.hpp \
    locality_finder.hpp \

SOURCES += \
    search_engine.cpp \
    intermediate_result.cpp \
    keyword_matcher.cpp \
    search_query.cpp \
    result.cpp \
    latlon_match.cpp \
    approximate_string_match.cpp \
    keyword_lang_matcher.cpp \
    params.cpp \
    house_detector.cpp \
    geometry_utils.cpp \
    locality_finder.cpp \
