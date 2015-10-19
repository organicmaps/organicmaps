# Map library.

TARGET = map
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

INCLUDEPATH *= $$ROOT_DIR/3party/protobuf/src $$ROOT_DIR/3party/expat/lib $$ROOT_DIR/3party/freetype/include

include($$ROOT_DIR/common.pri)

HEADERS += \
    framework.hpp \
    feature_vec_model.hpp \
    navigator.hpp \
    information_display.hpp \
    location_state.hpp \
    benchmark_provider.hpp \
    benchmark_engine.hpp \
    ruler.hpp \
    bookmark.hpp \
    styled_point.hpp \
    geourl_process.hpp \
    country_status_display.hpp \
    rotate_screen_task.hpp \
    compass_arrow.hpp \
    animator.hpp \
    move_screen_task.hpp \
    change_viewport_task.hpp \
    mwm_url.hpp \
    bookmark_manager.hpp \
    ge0_parser.hpp \
    track.hpp \
    alfa_animation_task.hpp \
    user_mark_container.hpp \
    user_mark.hpp \
    user_mark_dl_cache.hpp \
    anim_phase_chain.hpp \
    pin_click_manager.hpp \
    country_tree.hpp \
    active_maps_layout.hpp \
    navigator_utils.hpp \

SOURCES += \
    feature_vec_model.cpp \
    framework.cpp \
    navigator.cpp \
    information_display.cpp \
    location_state.cpp \
    benchmark_provider.cpp \
    benchmark_engine.cpp \
    ruler.cpp \
    address_finder.cpp \
    geourl_process.cpp \
    bookmark.cpp \
    styled_point.cpp \
    country_status_display.cpp \
    rotate_screen_task.cpp \
    compass_arrow.cpp \
    animator.cpp \
    move_screen_task.cpp \
    change_viewport_task.cpp \
    mwm_url.cpp \
    bookmark_manager.cpp \
    ge0_parser.cpp \
    ../api/src/c/api-client.c \
    track.cpp \
    alfa_animation_task.cpp \
    user_mark_container.cpp \
    user_mark_dl_cache.cpp \
    anim_phase_chain.cpp \
    pin_click_manager.cpp \
    country_tree.cpp \
    active_maps_layout.cpp \
    navigator_utils.cpp \

!iphone*:!tizen*:!android* {
  HEADERS += qgl_render_context.hpp
  SOURCES += qgl_render_context.cpp
  QT += opengl
}
