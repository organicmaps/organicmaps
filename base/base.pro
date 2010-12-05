# Base functions project.
TARGET = base
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES =

include($$ROOT_DIR/common.pri)

SOURCES += base.cpp \
    internal/debug_new.cpp \
    logging.cpp \
    thread.cpp \
    string_utils.cpp \
    profiler.cpp \
    commands_queue.cpp \

HEADERS += SRC_FIRST.hpp \
    assert.hpp \
    const_helper.hpp \
    internal/debug_new.hpp \
    internal/fast_mutex.hpp \
    math.hpp \
    pseudo_random.hpp \
    scope_guard.hpp \
    macros.hpp \
    base.hpp \
    src_point.hpp \
    bits.hpp \
    exception.hpp \
    internal/message.hpp \
    internal/fast_mutex.hpp \
    internal/debug_new.hpp \
    logging.hpp \
    start_mem_debug.hpp \
    stop_mem_debug.hpp \
    thread.hpp \
    mutex.hpp \
    casts.hpp \
    string_utils.hpp \
    rolling_hash.hpp \
    stl_add.hpp \
    timer.hpp \
    cache.hpp \
    profiler.hpp \
    threads_pool.hpp \
    matrix.hpp \
    set_operations.hpp \
    condition.hpp \
    commands_queue.hpp \
    ptr_utils.hpp \
    stats.hpp \
    monitor.hpp
