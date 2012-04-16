# Map library.

TARGET = map
TEMPLATE = lib
CONFIG += staticlib
INCLUDEPATH += ../3party/protobuf/src

ROOT_DIR = ..
DEPENDENCIES = search yg indexer geometry coding base expat

include($$ROOT_DIR/common.pri)

HEADERS += \
    framework.hpp \
    feature_vec_model.hpp \
    events.hpp \
    navigator.hpp \
    drawer_yg.hpp \
    draw_processor.hpp \
    draw_info.hpp \
    window_handle.hpp \
    tile_renderer.hpp \
    information_display.hpp \
    location_state.hpp \
    benchmark_provider.hpp \
    render_policy.hpp \
    tiling_render_policy_mt.hpp \
    tiling_render_policy_st.hpp \
    benchmark_tiling_render_policy_mt.hpp \
    benchmark_framework.hpp \
    framework_factory.hpp \
    render_policy_st.hpp \
    coverage_generator.hpp \
    tiler.hpp \
    tile.hpp \
    tile_cache.hpp \
    screen_coverage.hpp \
    basic_render_policy.hpp \
    render_queue.hpp \
    render_queue_routine.hpp \
    benchmark_render_policy_mt.hpp \
    ruler.hpp \
    measurement_utils.hpp \
    simple_render_policy.hpp \
    proto_to_yg_styles.hpp \
    test_render_policy.hpp \
    queued_renderer.hpp \
    basic_tiling_render_policy.hpp \
    render_policy_mt.hpp \
    bookmark.hpp \
    tile_set.hpp

SOURCES += \
    feature_vec_model.cpp \
    framework.cpp \
    navigator.cpp \
    drawer_yg.cpp \
    draw_processor.cpp \
    tile_renderer.cpp \
    information_display.cpp \
    location_state.cpp \
    benchmark_provider.cpp \
    render_policy.cpp \
    benchmark_tiling_render_policy_mt.cpp \
    tiling_render_policy_st.cpp \
    tiling_render_policy_mt.cpp \
    benchmark_framework.cpp \
    framework_factory.cpp \
    render_policy_st.cpp \
    coverage_generator.cpp \
    tiler.cpp \
    tile_cache.cpp \
    tile.cpp \
    screen_coverage.cpp \
    basic_render_policy.cpp \
    render_queue_routine.cpp \
    render_queue.cpp \
    benchmark_render_policy_mt.cpp \
    ruler.cpp \
    measurement_utils.cpp \
    window_handle.cpp \
    simple_render_policy.cpp \
    proto_to_yg_styles.cpp \
    test_render_policy.cpp \
    queued_renderer.cpp \
    events.cpp \
    basic_tiling_render_policy.cpp \
    render_policy_mt.cpp \
    address_finder.cpp \
    tile_set.cpp

!iphone*:!bada*:!android* {
  HEADERS += qgl_render_context.hpp
  SOURCES += qgl_render_context.cpp
  QT += opengl
}






