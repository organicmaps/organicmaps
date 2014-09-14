# Routing lib unit tests

TARGET = routing_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = routing indexer platform geometry coding base protobuf tomcrypt

macx-*: LIBS *= "-framework Foundation" "-framework IOKit"

include($$ROOT_DIR/common.pri)

linux*: QT *= core

SOURCES += \
  ../../testing/testingmain.cpp \
  features_road_graph_test.cpp \
  routing_smoke.cpp \
  road_graph_builder.cpp \
  road_graph_builder_test.cpp \
  dijkstra_router_test.cpp \
  vehicle_model_test.cpp \

HEADERS += \
  road_graph_builder.hpp \
  features_road_graph_test.hpp \

