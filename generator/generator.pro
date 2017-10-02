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
    cities_boundaries_builder.cpp \
    coastlines_generator.cpp \
    dumper.cpp \
    feature_builder.cpp \
    feature_generator.cpp \
    feature_merger.cpp \
    feature_sorter.cpp \
    metalines_builder.cpp \
    opentable_dataset.cpp \
    opentable_scoring.cpp \
    osm2meta.cpp \
    osm2type.cpp \
    osm_element.cpp \
    osm_id.cpp \
    osm_source.cpp \
    region_meta.cpp \
    restriction_collector.cpp \
    restriction_generator.cpp \
    restriction_writer.cpp \
    road_access_generator.cpp \
    routing_generator.cpp \
    routing_helpers.cpp \
    routing_index_generator.cpp \
    search_index_builder.cpp \
    sponsored_scoring.cpp \
    srtm_parser.cpp \
    statistics.cpp \
    tesselator.cpp \
    towns_dumper.cpp \
    traffic_generator.cpp \
    transit_generator.cpp \
    ugc_db.cpp \
    ugc_translator.cpp \
    unpack_mwm.cpp \
    utils.cpp \
    viator_dataset.cpp \

HEADERS += \
    altitude_generator.hpp \
    booking_dataset.hpp \
    borders_generator.hpp \
    borders_loader.hpp \
    centers_table_builder.hpp \
    check_model.hpp \
    cities_boundaries_builder.hpp \
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
    metalines_builder.hpp \
    opentable_dataset.hpp \
    osm2meta.hpp \
    osm2type.hpp \
    osm_element.hpp \
    osm_id.hpp \
    osm_o5m_source.hpp \
    osm_translator.hpp \
    osm_xml_source.hpp \
    polygonizer.hpp \
    region_meta.hpp \
    restriction_collector.hpp \
    restriction_generator.hpp \
    restriction_writer.hpp \
    road_access_generator.hpp \
    routing_generator.hpp \
    routing_helpers.hpp \
    routing_index_generator.hpp \
    search_index_builder.hpp \
    sponsored_dataset.hpp \
    sponsored_dataset_inl.hpp \
    sponsored_object_storage.hpp \
    sponsored_scoring.hpp \
    srtm_parser.hpp \
    statistics.hpp \
    tag_admixer.hpp \
    tesselator.hpp \
    towns_dumper.hpp \
    traffic_generator.hpp \
    transit_generator.hpp \
    ugc_db.hpp \
    ugc_translator.hpp \
    unpack_mwm.hpp \
    utils.hpp \
    viator_dataset.hpp \
    ways_merger.hpp \
    world_map_generator.hpp \
