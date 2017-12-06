TARGET = generator_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = generator_tests_support platform_tests_support generator drape_frontend routing \
               search storage ugc indexer drape map traffic routing_common platform editor geometry \
               coding base freetype expat jansson protobuf osrm stats_client \
               minizip succinct pugixml tess2 gflags oauthcpp stb_image sdf_image icu

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/jansson/src

LIBS *= -lsqlite3

QT *= core

macx-* {
  QT *= gui widgets # needed for QApplication with event loop, to test async events (downloader, etc.)
  LIBS *= "-framework IOKit" "-framework QuartzCore" "-framework Cocoa" "-framework SystemConfiguration"
}
win32*|linux* {
  QT *= network
}

INCLUDEPATH += $$ROOT_DIR/3party/jansson/src

HEADERS += \
    source_data.hpp \
    types_helper.hpp \

SOURCES += \
    ../../testing/testingmain.cpp \
    altitude_test.cpp \
    check_mwms.cpp \
    coasts_test.cpp \
    feature_builder_test.cpp \
    feature_merger_test.cpp \
    metadata_parser_test.cpp \
    osm2meta_test.cpp \
    osm_id_test.cpp \
    osm_o5m_source_test.cpp \
    osm_type_test.cpp \
    road_access_test.cpp \
    restriction_collector_test.cpp \
    restriction_test.cpp \
    source_data.cpp \
    source_to_element_test.cpp \
    srtm_parser_test.cpp \
    tag_admixer_test.cpp \
    tesselator_test.cpp \
    transit_tools.hpp \
    triangles_tree_coding_test.cpp \
    ugc_test.cpp \
