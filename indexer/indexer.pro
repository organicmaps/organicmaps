# Indexer library.

TARGET = indexer
TEMPLATE = lib
CONFIG += staticlib warn_on
INCLUDEPATH += ../3party/protobuf/src

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

SOURCES += \
    categories_holder.cpp \
    classificator.cpp \
    classificator_loader.cpp \
    coding_params.cpp \
    data_factory.cpp \
    data_header.cpp \
    drawing_rule_def.cpp \
    drawing_rules.cpp \
    drules_city_rank_table.cpp \
    drules_selector.cpp \
    drules_selector_parser.cpp \
    feature.cpp \
    feature_algo.cpp \
    feature_covering.cpp \
    feature_data.cpp \
    feature_decl.cpp \
    feature_impl.cpp \
    feature_loader.cpp \
    feature_loader_base.cpp \
    feature_utils.cpp \
    feature_visibility.cpp \
    features_offsets_table.cpp \
    features_vector.cpp \
    ftypes_matcher.cpp \
    geometry_coding.cpp \
    geometry_serialization.cpp \
    index.cpp \
    index_builder.cpp \
    map_style_reader.cpp \
    mercator.cpp \
    mwm_set.cpp \
    old/feature_loader_101.cpp \
    point_to_int64.cpp \
    scales.cpp \
    search_delimiters.cpp \
    search_index_builder.cpp \
    search_string_utils.cpp \
    types_mapping.cpp \

HEADERS += \
    categories_holder.hpp \
    cell_coverer.hpp \
    cell_id.hpp \
    classificator.hpp \
    classificator_loader.hpp \
    coding_params.hpp \
    data_factory.hpp \
    data_header.hpp \
    drawing_rule_def.hpp \
    drawing_rules.hpp \
    drules_city_rank_table.hpp \
    drules_include.hpp \
    drules_selector.cpp \
    drules_selector_parser.cpp \
    feature.hpp \
    feature_algo.hpp \
    feature_covering.hpp \
    feature_data.hpp \
    feature_decl.hpp \
    feature_impl.hpp \
    feature_loader.hpp \
    feature_loader_base.hpp \
    feature_meta.hpp \
    feature_processor.hpp \
    feature_utils.hpp \
    feature_visibility.hpp \
    features_offsets_table.hpp \
    features_vector.hpp \
    ftypes_matcher.hpp \
    geometry_coding.hpp \
    geometry_serialization.hpp \
    index.hpp \
    index_builder.hpp \
    interval_index.hpp \
    interval_index_builder.hpp \
    interval_index_iface.hpp \
    map_style.hpp \
    map_style_reader.hpp \
    mercator.hpp \
    mwm_set.hpp \
    old/feature_loader_101.hpp \
    old/interval_index_101.hpp \
    point_to_int64.hpp \
    scale_index.hpp \
    scale_index_builder.hpp \
    scales.hpp \
    search_delimiters.hpp \
    search_index_builder.hpp \
    search_string_utils.hpp \
    search_trie.hpp \
    string_file.hpp \
    string_file_values.hpp \
    tesselator_decl.hpp \
    tree_structure.hpp \
    types_mapping.hpp \
    unique_index.hpp \

OTHER_FILES += drules_struct.proto

SOURCES += drules_struct.pb.cc
HEADERS += drules_struct.pb.h
