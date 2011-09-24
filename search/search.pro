# Search library.

TARGET = search
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = indexer geometry coding base

include($$ROOT_DIR/common.pri)

HEADERS += \
    search_common.hpp \
    search_engine.hpp \
    intermediate_result.hpp \
    keyword_matcher.hpp \
    query.hpp \
    search_query.hpp \
    result.hpp \
    latlon_match.hpp \
    categories_holder.hpp \
    search_trie_matching.hpp \
    approximate_string_match.hpp \
    feature_match.hpp \

SOURCES += \
    search_engine.cpp \
    intermediate_result.cpp \
    keyword_matcher.cpp \
    query.cpp \
    search_query.cpp \
    result.cpp \
    latlon_match.cpp \
    categories_holder.cpp \
    search_trie_matching.cpp \
    approximate_string_match.cpp \














