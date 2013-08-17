# Base functions project.
TARGET = base
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

SOURCES += \
    base.cpp \
    logging.cpp \
    thread.cpp \
    string_utils.cpp \
    commands_queue.cpp \
    shared_buffer_manager.cpp \
    condition.cpp \
    lower_case.cpp \
    normalize_unicode.cpp \
    runner.cpp \
    timer.cpp \
    internal/message.cpp \
    exception.cpp \
    threaded_container.cpp \
    resource_pool.cpp \
    fence_manager.cpp \
    strings_bundle.cpp \
    string_format.cpp \
    object_tracker.cpp \
    scheduled_task.cpp \

HEADERS += \
    SRC_FIRST.hpp \
    assert.hpp \
    const_helper.hpp \
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
    logging.hpp \
    swap.hpp \
    thread.hpp \
    mutex.hpp \
    casts.hpp \
    string_utils.hpp \
    rolling_hash.hpp \
    stl_add.hpp \
    timer.hpp \
    cache.hpp \
    matrix.hpp \
    set_operations.hpp \
    condition.hpp \
    commands_queue.hpp \
    stats.hpp \
    shared_buffer_manager.hpp \
    buffer_vector.hpp \
    array_adapters.hpp \
    runner.hpp \
    mru_cache.hpp \
    threaded_container.hpp \
    threaded_list.hpp \
    resource_pool.hpp \
    limited_priority_queue.hpp \
    threaded_priority_queue.hpp \
    std_serialization.hpp \
    fence_manager.hpp \
    strings_bundle.hpp \
    string_format.hpp \
    object_tracker.hpp \
    regexp.hpp \
    scheduled_task.hpp \
