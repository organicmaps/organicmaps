# Base functions project.
TARGET = routing
TEMPLATE = lib
CONFIG += staticlib warn_on c++11

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

DEFINES *= BOOST_ERROR_CODE_HEADER_ONLY
INCLUDEPATH += $$ROOT_DIR/3party/jansson/src \
               $$ROOT_DIR/3party/osrm/osrm-backend/include \
               $$ROOT_DIR/3party/osrm/osrm-backend/third_party

SOURCES += \
    async_router.cpp \
    car_model.cpp \
    cross_mwm_road_graph.cpp \
    cross_mwm_router.cpp \
    cross_routing_context.cpp \
    features_road_graph.cpp \
    nearest_edge_finder.cpp \
    online_absent_fetcher.cpp \
    online_cross_fetcher.cpp \
    osrm2feature_map.cpp \
    osrm_engine.cpp \
    osrm_online_router.cpp \
    osrm_router.cpp \
    pedestrian_model.cpp \
    road_graph.cpp \
    road_graph_router.cpp \
    route.cpp \
    routing_algorithm.cpp \
    routing_mapping.cpp \
    turns.cpp \
    turns_generator.cpp \
    turns_sound.cpp \
    turns_sound_settings.cpp \
    vehicle_model.cpp \

HEADERS += \
    async_router.hpp \
    base/astar_algorithm.hpp \
    car_model.hpp \
    cross_mwm_road_graph.hpp \
    cross_mwm_router.hpp \
    cross_routing_context.hpp \
    features_road_graph.hpp \
    nearest_edge_finder.hpp \
    online_absent_fetcher.hpp \
    online_cross_fetcher.hpp \
    osrm2feature_map.hpp \
    osrm_data_facade.hpp \
    osrm_engine.hpp \
    osrm_online_router.hpp \
    osrm_router.hpp \
    pedestrian_model.hpp \
    road_graph.hpp \
    road_graph_router.hpp \
    route.hpp \
    router.hpp \
    routing_algorithm.hpp \
    routing_mapping.h \
    routing_settings.hpp \
    turns.hpp \
    turns_generator.hpp \
    turns_sound.hpp \
    turns_sound_settings.hpp \
    vehicle_model.hpp \
