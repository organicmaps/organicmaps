# Map library.

TARGET = map
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

INCLUDEPATH *= $$ROOT_DIR/3party/protobuf/src $$ROOT_DIR/3party/expat/lib $$ROOT_DIR/3party/freetype/include

include($$ROOT_DIR/common.pri)

HEADERS += \
    temp_text/text_engine.h \
    framework.hpp \
    feature_vec_model.hpp \
    events.hpp \
    navigator.hpp \
    drawer.hpp \
    feature_processor.hpp \
    path_info.hpp \
    window_handle.hpp \
    tile_renderer.hpp \
    information_display.hpp \
    location_state.hpp \
    benchmark_provider.hpp \
    render_policy.hpp \
    tiling_render_policy_mt.hpp \
    tiling_render_policy_st.hpp \
    benchmark_engine.hpp \
    coverage_generator.hpp \
    tiler.hpp \
    tile.hpp \
    tile_cache.hpp \
    ruler.hpp \
    simple_render_policy.hpp \
    proto_to_styles.hpp \
    queued_renderer.hpp \
    basic_tiling_render_policy.hpp \
    bookmark.hpp \
    tile_set.hpp \
    geourl_process.hpp \
    country_status_display.hpp \
    rotate_screen_task.hpp \
    compass_arrow.hpp \
    animator.hpp \
    move_screen_task.hpp \
    change_viewport_task.hpp \
    dialog_settings.hpp \
    mwm_url.hpp \
    feature_styler.hpp \
    feature_info.hpp \
    area_info.hpp \
    geometry_processors.hpp \
    bookmark_manager.hpp \
    ge0_parser.hpp \
    scales_processor.hpp \
    yopme_render_policy.hpp \
    track.hpp \
    alfa_animation_task.hpp \
    user_mark_container.hpp \
    user_mark.hpp \
    user_mark_dl_cache.hpp \
    anim_phase_chain.hpp \
    pin_click_manager.hpp \
    routing_session.hpp \
    country_tree.hpp \
    active_maps_layout.hpp \
    route_track.hpp \
    navigator_utils.hpp \
    software_renderer.hpp \
    gpu_drawer.hpp \
    cpu_drawer.hpp \
    frame_image.hpp \

SOURCES += \
    temp_text/text_engine.cpp \
    temp_text/default_font.cpp \
    feature_vec_model.cpp \
    framework.cpp \
    navigator.cpp \
    drawer.cpp \
    feature_processor.cpp \
    tile_renderer.cpp \
    information_display.cpp \
    location_state.cpp \
    benchmark_provider.cpp \
    render_policy.cpp \
    tiling_render_policy_st.cpp \
    tiling_render_policy_mt.cpp \
    benchmark_engine.cpp \
    coverage_generator.cpp \
    tiler.cpp \
    tile_cache.cpp \
    tile.cpp \
    ruler.cpp \
    window_handle.cpp \
    simple_render_policy.cpp \
    proto_to_styles.cpp \
    queued_renderer.cpp \
    events.cpp \
    basic_tiling_render_policy.cpp \
    address_finder.cpp \
    tile_set.cpp \
    geourl_process.cpp \
    bookmark.cpp \
    country_status_display.cpp \
    rotate_screen_task.cpp \
    compass_arrow.cpp \
    animator.cpp \
    move_screen_task.cpp \
    change_viewport_task.cpp \
    dialog_settings.cpp \
    mwm_url.cpp \
    feature_styler.cpp \
    feature_info.cpp \
    geometry_processors.cpp \
    bookmark_manager.cpp \
    ge0_parser.cpp \
    ../api/src/c/api-client.c \
    scales_processor.cpp \
    yopme_render_policy.cpp \
    track.cpp \
    alfa_animation_task.cpp \
    user_mark_container.cpp \
    user_mark_dl_cache.cpp \
    anim_phase_chain.cpp \
    pin_click_manager.cpp \
    routing_session.cpp \
    country_tree.cpp \
    active_maps_layout.cpp \
    route_track.cpp \
    navigator_utils.cpp \
    software_renderer.cpp \
    gpu_drawer.cpp \
    cpu_drawer.cpp \
    agg_curves.cpp \

!iphone*:!tizen*:!android* {
  HEADERS += qgl_render_context.hpp
  SOURCES += qgl_render_context.cpp
  QT += opengl
}
