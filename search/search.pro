# Search library.

TARGET = search
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

HEADERS += \
    algos.hpp \
    approximate_string_match.hpp \
    cancel_exception.hpp \
    cbv_ptr.hpp \
    dummy_rank_table.hpp \
    feature_offset_match.hpp \
    features_filter.hpp \
    features_layer.hpp \
    features_layer_matcher.hpp \
    features_layer_path_finder.hpp \
    geocoder.hpp \
    geometry_cache.hpp \
    geometry_utils.hpp \
    house_detector.hpp \
    house_numbers_matcher.hpp \
    house_to_street_table.hpp \
    intermediate_result.hpp \
    intersection_result.hpp \
    interval_set.hpp \
    keyword_lang_matcher.hpp \
    keyword_matcher.hpp \
    latlon_match.hpp \
    locality.hpp \
    locality_finder.hpp \
    locality_scorer.hpp \
    mode.hpp \
    mwm_context.hpp \
    nested_rects_cache.hpp \
    params.hpp \
    pre_ranker.hpp \
    pre_ranking_info.hpp \
    processor.hpp \
    processor_factory.hpp \
    projection_on_street.hpp \
    query_params.hpp \
    query_saver.hpp \
    rank_table_cache.hpp \
    ranking_info.hpp \
    ranking_utils.hpp \
    region.hpp \
    result.hpp \
    retrieval.hpp \
    reverse_geocoder.hpp \
    search_common.hpp \
    search_engine.hpp \
    search_index_values.hpp \
    search_model.hpp \
    search_string_intersection.hpp \
    search_trie.hpp \
    stats_cache.hpp \
    street_vicinity_loader.hpp \
    suggest.hpp \
    token_slice.hpp \
    types_skipper.hpp \

SOURCES += \
    approximate_string_match.cpp \
    cbv_ptr.cpp \
    dummy_rank_table.cpp \
    features_filter.cpp \
    features_layer.cpp \
    features_layer_matcher.cpp \
    features_layer_path_finder.cpp \
    geocoder.cpp \
    geometry_cache.cpp \
    geometry_utils.cpp \
    house_detector.cpp \
    house_numbers_matcher.cpp \
    house_to_street_table.cpp \
    intermediate_result.cpp \
    intersection_result.cpp \
    keyword_lang_matcher.cpp \
    keyword_matcher.cpp \
    latlon_match.cpp \
    locality.cpp \
    locality_finder.cpp \
    locality_scorer.cpp \
    mode.cpp \
    mwm_context.cpp \
    nested_rects_cache.cpp \
    params.cpp \
    pre_ranker.cpp \
    pre_ranking_info.cpp \
    processor.cpp \
    projection_on_street.cpp \
    query_params.cpp \
    query_saver.cpp \
    rank_table_cache.cpp \
    ranking_info.cpp \
    ranking_utils.cpp \
    region.cpp \
    result.cpp \
    retrieval.cpp \
    reverse_geocoder.cpp \
    search_engine.cpp \
    search_model.cpp \
    street_vicinity_loader.cpp \
    token_slice.cpp \
    types_skipper.cpp \
