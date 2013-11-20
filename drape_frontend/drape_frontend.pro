TARGET = drape_frontend
TEMPLATE = lib
CONFIG += staticlib

DEPENDENCIES = base
ROOT_DIR = ..
include($$ROOT_DIR/common.pri)

SOURCES += \
    backend_renderer.cpp \
    engine_context.cpp \
    memory_feature_index.cpp \
    message_queue.cpp \
    message.cpp \
    threads_commutator.cpp \
    message_acceptor.cpp \
    drop_tile_message.cpp \
    impl/backend_renderer_impl.cpp \
    update_coverage_message.cpp \
    resize_message.cpp \
    task_finish_message.cpp \
    read_mwm_task.cpp

HEADERS += \
    backend_renderer.hpp \
    render_thread.hpp \
    engine_context.hpp \
    memory_feature_index.hpp \
    tile_info.hpp \
    message_queue.hpp \
    message.hpp \
    threads_commutator.hpp \
    message_acceptor.hpp \
    drop_tile_message.hpp \
    impl/backend_renderer_impl.hpp \
    update_coverage_message.hpp \
    resize_message.hpp \
    task_finish_message.hpp \
    read_mwm_task.hpp \
    drop_coverage_message.hpp
