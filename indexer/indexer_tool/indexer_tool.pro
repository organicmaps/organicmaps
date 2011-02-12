# Indexer binary

ROOT_DIR = ../..
DEPENDENCIES = map storage indexer platform geometry coding base gflags expat sgitess version

include($$ROOT_DIR/common.pri)

CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

# needed for Platform::WorkingDir()
QT += core

win32 {
  LIBS += -lShell32
}

SOURCES += \
    indexer_tool.cpp \
    data_generator.cpp \
    feature_generator.cpp \
    feature_sorter.cpp \
    update_generator.cpp \
    grid_generator.cpp \
    statistics.cpp \
    kml_parser.cpp \
    feature_merger.cpp \

HEADERS += \
    osm_element.hpp \
    data_generator.hpp \
    feature_generator.hpp \
    first_pass_parser.hpp \
    data_cache_file.hpp \
    feature_sorter.hpp \
    update_generator.hpp \
    feature_bucketer.hpp \
    grid_generator.hpp \
    statistics.hpp \
    kml_parser.hpp \
    polygonizer.hpp \
    world_map_generator.hpp \
    feature_merger.hpp \
