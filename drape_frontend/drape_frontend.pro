TARGET = drape_frontend
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/protobuf/src
DEFINES += DRAW_INFO

SOURCES += \
    engine_context.cpp \
    memory_feature_index.cpp \
    message_queue.cpp \
    threads_commutator.cpp \
    message_acceptor.cpp \
    backend_renderer.cpp \
    read_mwm_task.cpp \
    batchers_pool.cpp \
    frontend_renderer.cpp \
    drape_engine.cpp \
    area_shape.cpp \
    read_manager.cpp \
    tile_info.cpp \
    stylist.cpp \
    line_shape.cpp \
    rule_drawer.cpp \
    viewport.cpp \
    tile_key.cpp \
    apply_feature_functors.cpp \
    visual_params.cpp \
    poi_symbol_shape.cpp \
    circle_shape.cpp \
    render_group.cpp \
    text_shape.cpp \
    path_text_shape.cpp \
    path_symbol_shape.cpp \
    text_layout.cpp \
    map_data_provider.cpp \
    user_mark_shapes.cpp \
    user_marks_provider.cpp \

HEADERS += \
    engine_context.hpp \
    memory_feature_index.hpp \
    tile_info.hpp \
    message_queue.hpp \
    message.hpp \
    threads_commutator.hpp \
    message_acceptor.hpp \
    backend_renderer.hpp \
    read_mwm_task.hpp \
    message_subclasses.hpp \
    map_shape.hpp \
    batchers_pool.hpp \
    frontend_renderer.hpp \
    drape_engine.hpp \
    area_shape.hpp \
    read_manager.hpp \
    stylist.hpp \
    line_shape.hpp \
    shape_view_params.hpp \
    rule_drawer.hpp \
    viewport.hpp \
    tile_key.hpp \
    apply_feature_functors.hpp \
    visual_params.hpp \
    poi_symbol_shape.hpp \
    circle_shape.hpp \
    render_group.hpp \
    text_shape.hpp \
    path_text_shape.hpp \
    path_symbol_shape.hpp \
    text_layout.hpp \
    intrusive_vector.hpp \
    map_data_provider.hpp \
    user_mark_shapes.hpp \
    user_marks_provider.hpp \
