# Library to use in Generator Tool

TARGET = generator
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = storage indexer geometry coding base expat

include($$ROOT_DIR/common.pri)

QT += core

SOURCES += \
    feature_merger.cpp \
    xml_element.cpp \
    data_generator.cpp \
    feature_generator.cpp \
    feature_sorter.cpp \
    update_generator.cpp \
    grid_generator.cpp \
    statistics.cpp \
    osm2type.cpp \
    classif_routine.cpp \
    borders_generator.cpp \
    osm_xml_parser.cpp \
    borders_loader.cpp \
    mwm_rect_updater.cpp

HEADERS += \
    feature_merger.hpp \
    xml_element.hpp \
    feature_bucketer.hpp \
    osm_element.hpp \
    data_generator.hpp \
    feature_generator.hpp \
    first_pass_parser.hpp \
    data_cache_file.hpp \
    feature_sorter.hpp \
    update_generator.hpp \
    grid_generator.hpp \
    statistics.hpp \
    polygonizer.hpp \
    world_map_generator.hpp \
    osm2type.hpp \
    classif_routine.hpp \
    borders_generator.hpp \
    osm_xml_parser.hpp \
    borders_loader.hpp \
    mwm_rect_updater.hpp \
    feature_emitter_iface.hpp \
