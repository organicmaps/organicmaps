# Routing lib unit tests

TARGET = routing_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = routing routing_common indexer platform_tests_support platform editor geometry coding base \
               osrm protobuf succinct jansson stats_client map traffic pugixml stats_client icu agg

macx-*: LIBS *= "-framework IOKit" "-framework SystemConfiguration"

include($$ROOT_DIR/common.pri)

QT *= core

INCLUDEPATH += $$ROOT_DIR/3party/jansson/src \
               $$ROOT_DIR/3party/osrm/osrm-backend/include \
               $$ROOT_DIR/3party/osrm/osrm-backend/third_party

SOURCES += \
  ../../testing/testingmain.cpp \
  applying_traffic_test.cpp \
  astar_algorithm_test.cpp \
  astar_progress_test.cpp \
  astar_router_test.cpp \
  async_router_test.cpp \
  checkpoint_predictor_test.cpp \
  coding_test.cpp \
  cross_mwm_connector_test.cpp \
  cross_routing_tests.cpp \
  cumulative_restriction_test.cpp \
  fake_graph_test.cpp \
  followed_polyline_test.cpp \
  index_graph_test.cpp \
  index_graph_tools.cpp \
  nearest_edge_finder_tests.cpp \
  online_cross_fetcher_test.cpp \
  osrm_router_test.cpp \
  restriction_test.cpp \
  road_access_test.cpp \
  road_graph_builder.cpp \
  road_graph_nearest_edges_test.cpp \
  route_tests.cpp \
  routing_helpers_tests.cpp \
  routing_mapping_test.cpp \
  routing_session_test.cpp \
  turns_generator_test.cpp \
  turns_sound_test.cpp \
  turns_tts_text_tests.cpp \

HEADERS += \
  index_graph_tools.hpp \
  road_graph_builder.hpp \
