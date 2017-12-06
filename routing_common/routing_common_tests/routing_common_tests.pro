# Routing common lib unit tests

TARGET = routing_common_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = routing_common indexer platform_tests_support platform editor geometry coding base \
               osrm protobuf succinct jansson stats_client map traffic pugixml stats_client icu agg

macx-*: LIBS *= "-framework IOKit" "-framework SystemConfiguration"

include($$ROOT_DIR/common.pri)

QT *= core

HEADERS += \
  transit_tools.hpp \

SOURCES += \
  ../../testing/testingmain.cpp \
  transit_graph_test.cpp \
  transit_json_parsing_test.cpp \
  transit_test.cpp
  transit_test.cpp \
  vehicle_model_for_country_test.cpp \
  vehicle_model_test.cpp \
