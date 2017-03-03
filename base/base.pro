# Base functions project.
TARGET = base
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

SOURCES += \
    base.cpp \
    condition.cpp \
    deferred_task.cpp \
    exception.cpp \
    gmtime.cpp \
    internal/message.cpp \
    levenshtein_dfa.cpp \
    logging.cpp \
    lower_case.cpp \
    normalize_unicode.cpp \
    random.cpp \
    shared_buffer_manager.cpp \
    src_point.cpp \
    string_format.cpp \
    string_utils.cpp \
    strings_bundle.cpp \
    sunrise_sunset.cpp \
    thread.cpp \
    thread_checker.cpp \
    thread_pool.cpp \
    threaded_container.cpp \
    timegm.cpp \
    timer.cpp \
    uni_string_dfa.cpp \

HEADERS += \
    SRC_FIRST.hpp \
    array_adapters.hpp \
    assert.hpp \
    base.hpp \
    bits.hpp \
    buffer_vector.hpp \
    cache.hpp \
    cancellable.hpp \
    checked_cast.hpp \
    collection_cast.hpp \
    condition.hpp \
    deferred_task.hpp \
    dfa_helpers.hpp \
    exception.hpp \
    gmtime.hpp \
    internal/message.hpp \
    levenshtein_dfa.hpp \
    limited_priority_queue.hpp \
    logging.hpp \
    macros.hpp \
    math.hpp \
    matrix.hpp \
    mem_trie.hpp \
    mutex.hpp \
    newtype.hpp \
    observer_list.hpp \
    random.hpp \
    range_iterator.hpp \
    ref_counted.hpp \
    regexp.hpp \
    rolling_hash.hpp \
    scope_guard.hpp \
    set_operations.hpp \
    shared_buffer_manager.hpp \
    small_set.hpp \
    src_point.hpp \
    stats.hpp \
    std_serialization.hpp \
    stl_add.hpp \
    stl_helpers.hpp \
    stl_iterator.hpp \
    string_format.hpp \
    string_utils.hpp \
    strings_bundle.hpp \
    sunrise_sunset.hpp \
    thread.hpp \
    thread_checker.hpp \
    thread_pool.hpp \
    threaded_container.hpp \
    threaded_list.hpp \
    threaded_priority_queue.hpp \
    timegm.hpp \
    timer.hpp \
    uni_string_dfa.hpp \
