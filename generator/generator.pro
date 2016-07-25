# Library to use in Generator Tool

TARGET = generator
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/gflags/src \
               $$ROOT_DIR/3party/osrm/osrm-backend/include \
               $$ROOT_DIR/3party/jansson/src

QT *= core

SOURCES += \
    altitude_generator.cpp \
    booking_dataset.cpp \
    booking_scoring.cpp \
    borders_generator.cpp \
    borders_loader.cpp \
    centers_table_builder.cpp \
    check_model.cpp \
    coastlines_generator.cpp \
    dumper.cpp \
    feature_builder.cpp \
    feature_generator.cpp \
    feature_merger.cpp \
    feature_sorter.cpp \
    osm2meta.cpp \
    osm2type.cpp \
    osm_element.cpp \
    osm_id.cpp \
    osm_source.cpp \
    region_meta.cpp \
    routing_generator.cpp \
    search_index_builder.cpp \
    srtm_parser.cpp \
    statistics.cpp \
    tesselator.cpp \
    towns_dumper.cpp \
    unpack_mwm.cpp \

HEADERS += \
    altitude_generator.hpp \
    booking_dataset.hpp \
    booking_scoring.hpp \
    borders_generator.hpp \
    borders_loader.hpp \
    centers_table_builder.hpp \
    check_model.hpp \
    coastlines_generator.hpp \
    dumper.hpp \
    feature_builder.hpp \
    feature_emitter_iface.hpp \
    feature_generator.hpp \
    feature_merger.hpp \
    feature_sorter.hpp \
    gen_mwm_info.hpp \
    generate_info.hpp \
    intermediate_data.hpp\
    intermediate_elements.hpp\
    osm2meta.hpp \
    osm2type.hpp \
    osm_element.hpp \
    osm_id.hpp \
    osm_o5m_source.hpp \
    osm_translator.hpp \
    osm_xml_source.hpp \
    polygonizer.hpp \
    region_meta.hpp \
    routing_generator.hpp \
    search_index_builder.hpp \
    srtm_parser.hpp \
    statistics.hpp \
    tag_admixer.hpp \
    tesselator.hpp \
    towns_dumper.hpp \
    unpack_mwm.hpp \
    ways_merger.hpp \
    world_map_generator.hpp \
