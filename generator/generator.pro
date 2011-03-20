# Library to use in Generator Tool

TARGET = generator
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = indexer geometry coding base expat

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
    kml_parser.cpp \
    osm2type.cpp \
    classif_routine.cpp \

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
    kml_parser.hpp \
    polygonizer.hpp \
    world_map_generator.hpp \
    osm2type.hpp \
    classif_routine.hpp \
