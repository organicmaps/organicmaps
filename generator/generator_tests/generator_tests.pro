TARGET = generator_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = generator map routing indexer platform geometry coding base expat sgitess protobuf tomcrypt osrm succinct

include($$ROOT_DIR/common.pri)

QT *= core

INCLUDEPATH *= $$ROOT_DIR/3party/expat/lib

HEADERS += \
    source_data.hpp \

SOURCES += \
    ../../testing/testingmain.cpp \
    check_mwms.cpp \
    classificator_tests.cpp \
    coasts_test.cpp \
    feature_builder_test.cpp \
    feature_merger_test.cpp \
    metadata_test.cpp \
    osm_id_test.cpp \
    osm_o5m_source_test.cpp \
    osm_parser_test.cpp \
    osm_type_test.cpp \
    tesselator_test.cpp \
    triangles_tree_coding_test.cpp \
    source_to_element_test.cpp \
    source_data.cpp \
