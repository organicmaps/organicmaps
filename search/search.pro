# Search library.

TARGET = search
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

HEADERS += \
    algos.hpp \
    approximate_string_match.hpp \
    categories_holder.hpp \
    dummy_rank_table.hpp \
    feature_offset_match.hpp \
    geometry_utils.hpp \
    house_detector.hpp \
    indexed_value.hpp \
    intermediate_result.hpp \
    interval_set.hpp \
    keyword_lang_matcher.hpp \
    keyword_matcher.hpp \
    latlon_match.hpp \
    locality.hpp \
    locality_finder.hpp \
    params.hpp \
    query_saver.hpp \
    region.hpp \
    result.hpp \
    retrieval.hpp \
    reverse_geocoder.hpp \
    search_common.hpp \
    search_delimiters.hpp \
    search_engine.hpp \
    search_index_values.hpp \
    search_query.hpp \
    search_query_factory.hpp \
    search_query_params.hpp \
    search_string_intersection.hpp \
    search_string_utils.hpp \
    search_trie.hpp \
    suggest.hpp \
    v2/features_layer.hpp \
    v2/features_layer_matcher.hpp \
    v2/features_layer_path_finder.hpp \
    v2/geocoder.hpp \
    v2/search_model.hpp \
    v2/search_query_v2.hpp \

SOURCES += \
    approximate_string_match.cpp \
    categories_holder.cpp \
    dummy_rank_table.cpp \
    geometry_utils.cpp \
    house_detector.cpp \
    intermediate_result.cpp \
    keyword_lang_matcher.cpp \
    keyword_matcher.cpp \
    latlon_match.cpp \
    locality.cpp \
    locality_finder.cpp \
    params.cpp \
    query_saver.cpp \
    region.cpp \
    result.cpp \
    retrieval.cpp \
    reverse_geocoder.cpp \
    search_delimiters.cpp \
    search_engine.cpp \
    search_query.cpp \
    search_query_params.cpp \
    search_string_utils.cpp \
    v2/features_layer.cpp \
    v2/features_layer_matcher.cpp \
    v2/features_layer_path_finder.cpp \
    v2/geocoder.cpp \
    v2/search_model.cpp \
    v2/search_query_v2.cpp \
