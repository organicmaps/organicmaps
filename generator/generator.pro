# Library to use in Generator Tool

TARGET = generator
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/gflags/src $$ROOT_DIR/3party/expat/lib \
               $$ROOT_DIR/3party/osrm/osrm-backend/Include

QT *= core

SOURCES += \
    feature_merger.cpp \
    xml_element.cpp \
    osm_source.cpp \
    feature_generator.cpp \
    feature_sorter.cpp \
    update_generator.cpp \
    statistics.cpp \
    osm2type.cpp \
    borders_generator.cpp \
    osm_xml_parser.cpp \
    borders_loader.cpp \
    dumper.cpp \
    unpack_mwm.cpp \
    feature_builder.cpp \
    osm_id.cpp \
    osm_decl.cpp \
    coastlines_generator.cpp \
    tesselator.cpp \
    check_model.cpp \
    routing_generator.cpp \

HEADERS += \
    feature_merger.hpp \
    xml_element.hpp \
    osm_element.hpp \
    feature_generator.hpp \
    first_pass_parser.hpp \
    data_cache_file.hpp \
    feature_sorter.hpp \
    update_generator.hpp \
    statistics.hpp \
    polygonizer.hpp \
    world_map_generator.hpp \
    osm2type.hpp \
    borders_generator.hpp \
    osm_xml_parser.hpp \
    borders_loader.hpp \
    feature_emitter_iface.hpp \
    dumper.hpp \
    generate_info.hpp \
    unpack_mwm.hpp \
    feature_builder.hpp \
    osm_id.hpp \
    osm_decl.hpp \
    coastlines_generator.hpp \
    tesselator.hpp \
    check_model.hpp \
    ways_merger.hpp \
    gen_mwm_info.hpp \
    routing_generator.hpp \
