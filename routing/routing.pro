# Routing project.
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
    checkpoint_predictor.cpp \
    checkpoints.cpp \
    cross_mwm_connector.cpp \
    cross_mwm_connector_serialization.cpp \
    cross_mwm_graph.cpp \
    cross_mwm_index_graph.cpp \
    cross_mwm_osrm_graph.cpp \
    cross_mwm_road_graph.cpp \
    cross_mwm_router.cpp \
    cross_routing_context.cpp \
    directions_engine.cpp \
    edge_estimator.cpp \
    features_road_graph.cpp \
    geometry.cpp \
    index_graph.cpp \
    index_graph_loader.cpp \
    index_graph_serialization.cpp \
    index_graph_starter.cpp \
    index_road_graph.cpp \
    index_router.cpp \
    joint.cpp \
    joint_index.cpp \
    nearest_edge_finder.cpp \
    online_absent_fetcher.cpp \
    online_cross_fetcher.cpp \
    osrm2feature_map.cpp \
    osrm_engine.cpp \
    pedestrian_directions.cpp \
    road_access.cpp \
    road_access_serialization.cpp \
    restriction_loader.cpp \
    restrictions_serialization.cpp \
    road_graph.cpp \
    road_graph_router.cpp \
    road_index.cpp \
    route.cpp \
    route_weight.cpp \
    router.cpp \
    router_delegate.cpp \
    routing_algorithm.cpp \
    routing_helpers.cpp \
    routing_mapping.cpp \
    routing_session.cpp \
    routing_settings.cpp \
    segmented_route.cpp \
    single_vehicle_world_graph.cpp \
    speed_camera.cpp \
    traffic_stash.cpp \
    transit_graph_loader.cpp \
    transit_world_graph.cpp \
    turns.cpp \
    turns_generator.cpp \
    turns_notification_manager.cpp \
    turns_sound_settings.cpp \
    turns_tts_text.cpp \
    vehicle_mask.cpp \    
    world_graph.cpp \

HEADERS += \
    async_router.hpp \
    base/astar_algorithm.hpp \
    base/astar_weight.hpp \
    base/followed_polyline.hpp \
    base/routing_result.hpp \
    bicycle_directions.hpp \
    checkpoint_predictor.hpp \
    checkpoints.hpp \
    coding.hpp \
    cross_mwm_connector.hpp \
    cross_mwm_connector_serialization.hpp \
    cross_mwm_graph.hpp \
    cross_mwm_index_graph.hpp \
    cross_mwm_osrm_graph.hpp \
    cross_mwm_road_graph.hpp \
    cross_mwm_router.hpp \
    cross_routing_context.hpp \
    directions_engine.hpp \
    edge_estimator.hpp \
    fake_edges_container.hpp \
    fake_graph.hpp \
    fake_vertex.hpp \
    features_road_graph.hpp \
    geometry.hpp \
    index_graph.hpp \
    index_graph_loader.hpp \
    index_graph_serialization.hpp \
    index_graph_starter.hpp \
    index_road_graph.hpp \
    index_router.hpp \
    joint.hpp \
    joint_index.hpp \
    loaded_path_segment.hpp \
    nearest_edge_finder.hpp \
    num_mwm_id.hpp \
    online_absent_fetcher.hpp \
    online_cross_fetcher.hpp \
    osrm2feature_map.hpp \
    osrm_data_facade.hpp \
    osrm_engine.hpp \
    pedestrian_directions.hpp \
    restriction_loader.hpp \
    restrictions_serialization.hpp \
    road_access.hpp \
    road_access_serialization.hpp \
    road_graph.hpp \
    road_graph_router.hpp \
    road_index.hpp \
    road_point.hpp \
    route.hpp \
    route_point.hpp \
    route_weight.hpp \
    router.hpp \
    router_delegate.hpp \
    routing_algorithm.hpp \
    routing_exceptions.hpp \
    routing_helpers.hpp \
    routing_mapping.hpp \
    routing_result_graph.hpp \
    routing_session.hpp \
    routing_settings.hpp \
    segment.hpp \
    segmented_route.hpp \
    single_vehicle_world_graph.hpp \
    speed_camera.hpp \
    traffic_stash.hpp \
    transit_graph.hpp \
    transit_graph_loader.hpp \
    transit_world_graph.hpp \
    transition_points.hpp \
    turn_candidate.hpp \
    turns.hpp \
    turns_generator.hpp \
    turns_notification_manager.hpp \
    turns_sound_settings.hpp \
    turns_tts_text.hpp \
    vehicle_mask.hpp \
    world_graph.hpp \
