# Indexer library.

TARGET = indexer
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = search geometry coding base expat

include($$ROOT_DIR/common.pri)


SOURCES += \
    classificator.cpp \
    drawing_rules.cpp \
    drawing_rule_def.cpp \
    scales.cpp \
    osm_decl.cpp \
    feature.cpp \
    classificator_loader.cpp \
    scale_index.cpp \
    covering.cpp \
    point_to_int64.cpp \
    mercator.cpp \
    index_builder.cpp \
    feature_visibility.cpp \
    data_header.cpp \
    geometry_coding.cpp \
    geometry_serialization.cpp \
    tesselator.cpp \
    feature_data.cpp \
    feature_rect.cpp \
    types_mapping.cpp \
    search_index_builder.cpp \
    data_factory.cpp \
    old/feature_loader_101.cpp \
    coding_params.cpp \
    feature_loader_base.cpp \
    feature_loader.cpp \

HEADERS += \
    feature.hpp \
    cell_coverer.hpp \
    cell_id.hpp \
    classificator.hpp \
    drawing_rules.hpp \
    drawing_rule_def.hpp \
    features_vector.hpp \
    scale_index.hpp \
    scale_index_builder.hpp \
    index.hpp \
    index_builder.hpp \
    scales.hpp \
    osm_decl.hpp \
    classificator_loader.hpp \
    interval_index.hpp \
    interval_index_builder.hpp \
    covering.hpp \
    mercator.hpp \
    feature_processor.hpp \
    file_reader_stream.hpp \
    file_writer_stream.hpp \
    feature_visibility.hpp \
    data_header.hpp \
    tree_structure.hpp \
    feature_impl.hpp \
    geometry_coding.hpp \
    geometry_serialization.hpp \
    point_to_int64.hpp \
    tesselator.hpp \
    tesselator_decl.hpp \
    feature_data.hpp \
    feature_rect.hpp \
    types_mapping.hpp \
    search_index_builder.hpp \
    interval_index_iface.hpp \
    data_factory.hpp \
    old/interval_index_101.hpp \
    old/feature_loader_101.hpp \
    coding_params.hpp \
    feature_loader_base.hpp \
    feature_loader.hpp \
