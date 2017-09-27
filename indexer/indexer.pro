# Indexer library.

TARGET = indexer
TEMPLATE = lib
CONFIG += staticlib warn_on
INCLUDEPATH += ../3party/protobuf/protobuf/src

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

SOURCES += \
    altitude_loader.cpp \
    categories_holder.cpp \
    categories_holder_loader.cpp \
    categories_index.cpp \
    centers_table.cpp \
    classificator.cpp \
    classificator_loader.cpp \
    coding_params.cpp \
    cuisines.cpp \
    data_factory.cpp \
    data_header.cpp \
    drawing_rule_def.cpp \
    drawing_rules.cpp \
    drules_selector.cpp \
    drules_selector_parser.cpp \
    editable_map_object.cpp \
    edits_migration.cpp \
    feature.cpp \
    feature_algo.cpp \
    feature_covering.cpp \
    feature_data.cpp \
    feature_decl.cpp \
    feature_impl.cpp \
    feature_loader.cpp \
    feature_loader_base.cpp \
    feature_meta.cpp \
    feature_utils.cpp \
    feature_visibility.cpp \
    features_offsets_table.cpp \
    features_vector.cpp \
    ftypes_matcher.cpp \
    geometry_coding.cpp \
    geometry_serialization.cpp \
    index.cpp \
    index_builder.cpp \
    index_helpers.cpp \
    map_object.cpp \
    map_style.cpp \
    map_style_reader.cpp \
    mwm_set.cpp \
    new_feature_categories.cpp \  # it's in indexer because of CategoriesHolder dependency.
    old/feature_loader_101.cpp \
    osm_editor.cpp \
    postcodes_matcher.cpp \  # it's in indexer due to editor wich is in indexer and depends on postcodes_marcher
    rank_table.cpp \
    road_shields_parser.cpp \
    scales.cpp \
    search_delimiters.cpp \    # it's in indexer because of CategoriesHolder dependency.
    search_string_utils.cpp \  # it's in indexer because of CategoriesHolder dependency.
    string_slice.cpp \
    types_mapping.cpp \

HEADERS += \
    altitude_loader.hpp \
    boundary_boxes.hpp \
    boundary_boxes_serdes.hpp \
    categories_holder.hpp \
    categories_index.hpp \
    cell_coverer.hpp \
    cell_id.hpp \
    centers_table.hpp \
    classificator.hpp \
    classificator_loader.hpp \
    coding_params.hpp \
    cuisines.hpp \
    data_factory.hpp \
    data_header.hpp \
    drawing_rule_def.hpp \
    drawing_rules.hpp \
    drules_include.hpp \
    drules_selector.hpp \
    drules_selector_parser.hpp \
    editable_map_object.hpp \
    edits_migration.hpp \
    feature.hpp \
    feature_algo.hpp \
    feature_altitude.hpp \
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
    ftraits.hpp \
    ftypes_mapping.hpp \
    ftypes_matcher.hpp \
    geometry_coding.hpp \
    geometry_serialization.hpp \
    index.hpp \
    index_builder.hpp \
    index_helpers.hpp \
    interval_index.hpp \
    interval_index_builder.hpp \
    interval_index_iface.hpp \
    map_object.hpp \
    map_style.hpp \
    map_style_reader.hpp \
    mwm_set.hpp \
    new_feature_categories.hpp \  # it's in indexer because of CategoriesHolder dependency.
    old/feature_loader_101.hpp \
    old/interval_index_101.hpp \
    osm_editor.hpp \
    postcodes_matcher.hpp \   # it's in indexer due to editor wich is in indexer and depends on postcodes_marcher
    rank_table.hpp \
    road_shields_parser.hpp \
    scale_index.hpp \
    scale_index_builder.hpp \
    scales.hpp \
    search_delimiters.hpp \      # it's in indexer because of CategoriesHolder dependency.
    search_string_utils.hpp \    # it's in indexer because of CategoriesHolder dependency.
    string_set.hpp \
    string_slice.hpp \
    succinct_trie_builder.hpp \
    succinct_trie_reader.hpp \
    scales_patch.hpp \
    tesselator_decl.hpp \
    tree_structure.hpp \
    trie.hpp \
    trie_builder.hpp \
    trie_reader.hpp \
    types_mapping.hpp \
    unique_index.hpp \

OTHER_FILES += drules_struct.proto

SOURCES += drules_struct.pb.cc
HEADERS += drules_struct.pb.h
