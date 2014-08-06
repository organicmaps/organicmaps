# Base functions project.
TARGET = routing
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

INCLUDEPATH += $$ROOT_DIR/3party/jansson/src

SOURCES += \
    route.cpp \
    routing_engine.cpp \
    road_graph.cpp \
    helicopter_router.cpp \
    osrm_online_router.cpp \
    road_graph_router.cpp \
    dijkstra_router.cpp \
    features_road_graph.cpp \
    vehicle_model.cpp \

HEADERS += \
    route.hpp \
    routing_engine.hpp \
    router.hpp \
    road_graph.hpp \
    helicopter_router.hpp \
    osrm_online_router.hpp \
    road_graph_router.hpp \
    dijkstra_router.hpp \
    features_road_graph.hpp \
    vehicle_model.hpp \
