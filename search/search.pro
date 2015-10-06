# Search library.

TARGET = search
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

HEADERS += \
    algos.hpp \
    approximate_string_match.hpp \
    feature_offset_match.hpp \
    geometry_utils.hpp \
    house_detector.hpp \
    indexed_value.hpp \
    intermediate_result.hpp \
    keyword_lang_matcher.hpp \
    keyword_matcher.hpp \
    latlon_match.hpp \
    locality_finder.hpp \
    params.hpp \
    query_saver.hpp \
    result.hpp \
    retrieval.hpp \
    search_common.hpp \
    search_engine.hpp \
    search_query.hpp \
    search_query_factory.hpp \
    search_query_params.hpp \
    search_string_intersection.hpp \
    suggest.hpp \

SOURCES += \
    approximate_string_match.cpp \
    geometry_utils.cpp \
    house_detector.cpp \
    intermediate_result.cpp \
    keyword_lang_matcher.cpp \
    keyword_matcher.cpp \
    latlon_match.cpp \
    locality_finder.cpp \
    params.cpp \
    query_saver.cpp \
    result.cpp \
    retrieval.cpp \
    search_engine.cpp \
    search_query.cpp \
    search_query_params.cpp \
