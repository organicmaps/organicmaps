# This subproject implements integration tests.
# This tests are launched on the whole world dataset.

# It is recommended to place tests here in the following cases:
# - tests are written to be launch on the whole world dataset;
# - tests covers significant number of subsystems;

TARGET = routing_integration_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = map routing traffic routing_common search storage mwm_diff ugc indexer platform editor geometry \
               coding base osrm jansson protobuf bsdiff succinct stats_client pugixml icu agg

DEPENDENCIES += opening_hours

!iphone*:!android*:!tizen:!macx-* {
  QT *= network
}

macx-*: LIBS *= "-framework IOKit" "-framework SystemConfiguration"

include($$ROOT_DIR/common.pri)

QT *= core

SOURCES += \
  ../../testing/testingmain.cpp \
  bicycle_route_test.cpp \
  bicycle_turn_test.cpp \
  cross_section_tests.cpp \
  get_altitude_test.cpp \
  online_cross_tests.cpp \
  pedestrian_route_test.cpp \
  road_graph_tests.cpp \
  route_test.cpp \
  routing_test_tools.cpp \
  street_names_test.cpp \
  turn_test.cpp \

HEADERS += \
  routing_test_tools.hpp \
