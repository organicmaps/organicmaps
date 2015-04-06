# Base functions project.
TARGET = routing
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

DEFINES *= BOOST_ERROR_CODE_HEADER_ONLY
INCLUDEPATH += $$ROOT_DIR/3party/jansson/src \
               $$ROOT_DIR/3party/osrm/osrm-backend/Include

SOURCES += \
    cross_routing_context.cpp \
    osrm_router.cpp \
    osrm_online_router.cpp \
    osrm2feature_map.cpp \
    route.cpp \
    routing_mapping.cpp \
    turns.cpp \
    vehicle_model.cpp \
    astar_router.cpp \
    dijkstra_router.cpp \
    features_road_graph.cpp \
    road_graph_router.cpp \
    road_graph.cpp

HEADERS += \
    cross_routing_context.hpp \
    osrm_router.hpp \
    osrm_online_router.hpp \
    osrm_data_facade.hpp \
    osrm2feature_map.hpp \
    route.hpp \
    router.hpp \
    routing_mapping.h \
    turns.hpp \
    vehicle_model.hpp \
    astar_router.hpp \
    dijkstra_router.hpp \
    features_road_graph.hpp \
    road_graph_router.hpp \
    road_graph.hpp
