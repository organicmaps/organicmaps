# Base functions project.
TARGET = base
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES =

include($$ROOT_DIR/common.pri)

SOURCES += \
    base.cpp \
    internal/debug_new.cpp \
    logging.cpp \
    thread.cpp \
    string_utils.cpp \
    profiler.cpp \
    commands_queue.cpp \
    shared_buffer_manager.cpp \
    memory_mapped_file.cpp \
    path_utils.cpp \
    condition.cpp \
    lower_case.cpp \
    normalize_unicode.cpp \
    runner.cpp \
    timer.cpp \

HEADERS += \
    SRC_FIRST.hpp \
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
    swap.hpp \
    thread.hpp \
    mutex.hpp \
    casts.hpp \
    string_utils.hpp \
    rolling_hash.hpp \
    stl_add.hpp \
    timer.hpp \
    cache.hpp \
    profiler.hpp \
    matrix.hpp \
    set_operations.hpp \
    condition.hpp \
    commands_queue.hpp \
    stats.hpp \
    shared_buffer_manager.hpp \
    memory_mapped_file.hpp \
    buffer_vector.hpp \
    path_utils.hpp \
    array_adapters.hpp \
    runner.hpp \
    object_pool.hpp
