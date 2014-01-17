TARGET = drape_frontend
TEMPLATE = lib
CONFIG += staticlib

DEPENDENCIES = drape base
ROOT_DIR = ..
include($$ROOT_DIR/common.pri)

DEFINES += DRAW_INFO

SOURCES += \
    engine_context.cpp \
    memory_feature_index.cpp \
    message_queue.cpp \
    message.cpp \
    threads_commutator.cpp \
    message_acceptor.cpp \
    backend_renderer.cpp \
    read_mwm_task.cpp \
    batchers_pool.cpp \
    frontend_renderer.cpp \
    drape_engine.cpp \
    area_shape.cpp

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
    area_shape.hpp
