# Map library.

TARGET = map
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

INCLUDEPATH *= $$ROOT_DIR/3party/protobuf/src $$ROOT_DIR/3party/freetype/include

include($$ROOT_DIR/common.pri)

HEADERS += \
    active_maps_layout.hpp \
    alfa_animation_task.hpp \
    anim_phase_chain.hpp \
    animator.hpp \
    bookmark.hpp \
    bookmark_manager.hpp \
    change_viewport_task.hpp \
    compass_arrow.hpp \
    country_tree.hpp \
    feature_vec_model.hpp \
    framework.hpp \
    ge0_parser.hpp \
    geourl_process.hpp \
    mwm_url.hpp \
    pin_click_manager.hpp \
    route_track.hpp \
    routing_session.hpp \
    storage_bridge.hpp \
    track.hpp \
    user_mark.hpp \
    user_mark_container.hpp \

SOURCES += \
   ../api/src/c/api-client.c \
    active_maps_layout.cpp \
    address_finder.cpp \
    alfa_animation_task.cpp \
    anim_phase_chain.cpp \
    animator.cpp \
    bookmark.cpp \
    bookmark_manager.cpp \
    change_viewport_task.cpp \
    compass_arrow.cpp \
    country_tree.cpp \
    feature_vec_model.cpp \
    framework.cpp \
    ge0_parser.cpp \
    geourl_process.cpp \
    mwm_url.cpp \
    pin_click_manager.cpp \
    route_track.cpp \
    routing_session.cpp \
    storage_bridge.cpp \
    track.cpp \
    user_mark.cpp \
    user_mark_container.cpp \

!iphone*:!tizen*:!android* {
  QT += opengl
}
