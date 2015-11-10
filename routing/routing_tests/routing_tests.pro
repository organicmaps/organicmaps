 # Routing lib unit tests

TARGET = routing_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = routing indexer platform_tests_support platform geometry coding base \
               osrm protobuf tomcrypt succinct jansson stats_client map

macx-*: LIBS *= "-framework IOKit" "-framework SystemConfiguration"

include($$ROOT_DIR/common.pri)

QT *= core

INCLUDEPATH += $$ROOT_DIR/3party/jansson/src \
               $$ROOT_DIR/3party/osrm/osrm-backend/include \
               $$ROOT_DIR/3party/osrm/osrm-backend/third_party

SOURCES += \
  ../../testing/testingmain.cpp \
  astar_algorithm_test.cpp \
  astar_progress_test.cpp \
  astar_router_test.cpp \
  async_router_test.cpp \
  cross_routing_tests.cpp \
  followed_polyline_test.cpp \
  nearest_edge_finder_tests.cpp \
  online_cross_fetcher_test.cpp \
  osrm_router_test.cpp \
  road_graph_builder.cpp \
  road_graph_nearest_edges_test.cpp \
  route_tests.cpp \
  routing_mapping_test.cpp \
  routing_session_test.cpp \
  turns_generator_test.cpp \
  turns_sound_test.cpp \
  turns_tts_text_tests.cpp \
  vehicle_model_test.cpp \

HEADERS += \
  road_graph_builder.hpp \
