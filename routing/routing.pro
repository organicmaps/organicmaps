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
    bicycle_directions.cpp \
    bicycle_model.cpp \
    car_model.cpp \
    car_router.cpp \
    cross_mwm_road_graph.cpp \
    cross_mwm_router.cpp \
    cross_routing_context.cpp \
    directions_engine.cpp \
    edge_estimator.cpp \
    features_road_graph.cpp \
    geometry.cpp \
    index_graph.cpp \
    index_graph_starter.cpp \
    joint.cpp \
    joint_index.cpp \
    nearest_edge_finder.cpp \
    online_absent_fetcher.cpp \
    online_cross_fetcher.cpp \
    osrm2feature_map.cpp \
    osrm_engine.cpp \
    osrm_helpers.cpp \
    osrm_path_segment_factory.cpp \
    pedestrian_directions.cpp \
    pedestrian_model.cpp \
    restriction_loader.cpp \
    road_graph.cpp \
    road_graph_router.cpp \
    road_index.cpp \
    route.cpp \
    router.cpp \
    router_delegate.cpp \
    routing_algorithm.cpp \
    routing_helpers.cpp \
    routing_mapping.cpp \
    routing_serialization.cpp \
    routing_session.cpp \
    single_mwm_router.cpp \
    speed_camera.cpp \
    turns.cpp \
    turns_generator.cpp \
    turns_notification_manager.cpp \
    turns_sound_settings.cpp \
    turns_tts_text.cpp \
    vehicle_model.cpp \


HEADERS += \
    async_router.hpp \
    base/astar_algorithm.hpp \
    base/followed_polyline.hpp \
    bicycle_directions.hpp \
    bicycle_model.hpp \
    car_model.hpp \
    car_router.hpp \
    cross_mwm_road_graph.hpp \
    cross_mwm_router.hpp \
    cross_routing_context.hpp \
    directions_engine.hpp \
    edge_estimator.hpp \
    features_road_graph.hpp \
    geometry.hpp \
    index_graph.hpp \
    index_graph_starter.hpp \
    joint.hpp \
    joint_index.hpp \
    loaded_path_segment.hpp \
    nearest_edge_finder.hpp \
    online_absent_fetcher.hpp \
    online_cross_fetcher.hpp \
    osrm2feature_map.hpp \
    osrm_data_facade.hpp \
    osrm_engine.hpp \
    osrm_helpers.hpp \
    osrm_path_segment_factory.hpp \
    pedestrian_directions.hpp \
    pedestrian_model.hpp \
    restriction_loader.hpp \
    road_graph.hpp \
    road_graph_router.hpp \
    road_index.hpp \
    road_point.hpp \
    route.hpp \
    router.hpp \
    router_delegate.hpp \
    routing_algorithm.hpp \
    routing_exception.hpp \
    routing_helpers.hpp \
    routing_mapping.hpp \
    routing_result_graph.hpp \
    routing_serialization.hpp \
    routing_session.hpp \
    routing_settings.hpp \
    single_mwm_router.hpp \
    speed_camera.hpp \
    turn_candidate.hpp \
    turns.hpp \
    turns_generator.hpp \
    turns_notification_manager.hpp \
    turns_sound_settings.hpp \
    turns_tts_text.hpp \
    vehicle_model.hpp \
