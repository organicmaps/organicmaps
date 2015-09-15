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
    base/followed_polyline.cpp \
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
    osrm_router.cpp \
    pedestrian_directions.cpp \
    pedestrian_model.cpp \
    road_graph.cpp \
    road_graph_router.cpp \
    route.cpp \
    router.cpp \
    router_delegate.cpp \
    routing_algorithm.cpp \
    routing_mapping.cpp \
    routing_session.cpp \
    speed_camera.cpp \
    turns.cpp \
    turns_generator.cpp \
    turns_sound.cpp \
    turns_sound_settings.cpp \
    turns_tts_text.cpp \
    vehicle_model.cpp \

HEADERS += \
    async_router.hpp \
    base/astar_algorithm.hpp \
    base/followed_polyline.hpp \
    car_model.hpp \
    cross_mwm_road_graph.hpp \
    cross_mwm_router.hpp \
    cross_routing_context.hpp \
    directions_engine.hpp \
    features_road_graph.hpp \
    nearest_edge_finder.hpp \
    online_absent_fetcher.hpp \
    online_cross_fetcher.hpp \
    osrm2feature_map.hpp \
    osrm_data_facade.hpp \
    osrm_engine.hpp \
    osrm_router.hpp \
    pedestrian_directions.hpp \
    pedestrian_model.hpp \
    road_graph.hpp \
    road_graph_router.hpp \
    route.hpp \
    router.hpp \
    router_delegate.hpp \
    routing_algorithm.hpp \
    routing_mapping.hpp \
    routing_session.hpp \
    routing_settings.hpp \
    speed_camera.hpp \
    turns.hpp \
    turns_generator.hpp \
    turns_sound.hpp \
    turns_sound_settings.hpp \
    turns_tts_text.hpp \
    vehicle_model.hpp \
