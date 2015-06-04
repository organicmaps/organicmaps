TARGET = drape_frontend
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/protobuf/src
#DEFINES += DRAW_INFO

SOURCES += \
    animation/base_interpolator.cpp \
    animation/interpolation_holder.cpp \
    animation/interpolations.cpp \
    animation/model_view_animation.cpp \
    animation/opacity_animation.cpp \
    animation/show_hide_animation.cpp \
    apply_feature_functors.cpp \
    area_shape.cpp \
    backend_renderer.cpp \
    base_renderer.cpp \
    batchers_pool.cpp \
    circle_shape.cpp \
    drape_engine.cpp \
    engine_context.cpp \
    frontend_renderer.cpp \
    line_shape.cpp \
    line_shape_helper.cpp \
    map_data_provider.cpp \
    memory_feature_index.cpp \
    message_acceptor.cpp \
    message_queue.cpp \
    my_position.cpp \
    my_position_controller.cpp \
    navigator.cpp \
    path_symbol_shape.cpp \
    path_text_shape.cpp \
    poi_symbol_shape.cpp \
    read_manager.cpp \
    read_mwm_task.cpp \
    render_group.cpp \
    render_node.cpp \
    route_builder.cpp \
    route_renderer.cpp \
    route_shape.cpp \
    rule_drawer.cpp \
    selection_shape.cpp \
    stylist.cpp \
    text_layout.cpp \
    text_shape.cpp \
    threads_commutator.cpp \
    tile_info.cpp \
    tile_key.cpp \
    tile_tree.cpp \
    tile_tree_builder.cpp \
    tile_utils.cpp \
    user_event_stream.cpp \
    user_mark_shapes.cpp \
    user_marks_provider.cpp \
    viewport.cpp \
    visual_params.cpp \

HEADERS += \
    animation/base_interpolator.hpp \
    animation/interpolation_holder.hpp \
    animation/interpolations.hpp \
    animation/model_view_animation.hpp \
    animation/opacity_animation.hpp \
    animation/show_hide_animation.hpp \
    animation/value_mapping.hpp \
    apply_feature_functors.hpp \
    area_shape.hpp \
    backend_renderer.hpp \
    base_renderer.hpp \
    batchers_pool.hpp \
    circle_shape.hpp \
    drape_engine.hpp \
    engine_context.hpp \
    frontend_renderer.hpp \
    intrusive_vector.hpp \
    line_shape.hpp \
    line_shape_helper.hpp \
    map_data_provider.hpp \
    map_shape.hpp \
    memory_feature_index.hpp \
    message.hpp \
    message_acceptor.hpp \
    message_queue.hpp \
    message_subclasses.hpp \
    my_position.hpp \
    my_position_controller.hpp \
    navigator.hpp \
    path_symbol_shape.hpp \
    path_text_shape.hpp \
    poi_symbol_shape.hpp \
    read_manager.hpp \
    read_mwm_task.hpp \
    render_group.hpp \
    render_node.hpp \
    route_builder.hpp \
    route_renderer.hpp \
    route_shape.hpp \
    rule_drawer.hpp \
    selection_shape.hpp \
    shape_view_params.hpp \
    stylist.hpp \
    text_layout.hpp \
    text_shape.hpp \
    threads_commutator.hpp \
    tile_info.hpp \
    tile_key.hpp \
    tile_tree.hpp \
    tile_tree_builder.hpp \
    tile_utils.hpp \
    user_event_stream.hpp \
    user_mark_shapes.hpp \
    user_marks_provider.hpp \
    viewport.hpp \
    visual_params.hpp \
