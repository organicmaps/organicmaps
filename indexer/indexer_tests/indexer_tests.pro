TARGET = indexer_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = indexer platform editor geometry coding base protobuf tomcrypt \
               succinct opening_hours pugixml stats_client \

include($$ROOT_DIR/common.pri)

QT *= core

HEADERS += \
    test_mwm_set.hpp \
    test_polylines.hpp \

SOURCES += \
    ../../testing/testingmain.cpp \
    categories_test.cpp \
    cell_coverer_test.cpp \
    cell_id_test.cpp \
    checker_test.cpp \
    drules_selector_parser_test.cpp \
    editable_map_object_test.cpp \
    feature_metadata_test.cpp \
    feature_xml_test.cpp \
    features_offsets_table_test.cpp \
    features_vector_test.cpp \
    geometry_coding_test.cpp \
    geometry_serialization_test.cpp \
    index_builder_test.cpp \
    index_test.cpp \
    interval_index_test.cpp \
    mwm_set_test.cpp \
    point_to_int64_test.cpp \
    postcodes_matcher_tests.cpp \
    rank_table_test.cpp \
    scales_test.cpp \
    search_string_utils_test.cpp \
    sort_and_merge_intervals_test.cpp \
    string_slice_tests.cpp \
    succinct_trie_test.cpp \
    test_polylines.cpp \
    test_type.cpp \
    trie_test.cpp \
    visibility_test.cpp \
