#-------------------------------------------------
#
# Project created by QtCreator 2013-09-17T15:41:48
#
#-------------------------------------------------

TARGET = drape
TEMPLATE = lib
CONFIG += staticlib warn_on

DEPENDENCIES = base

ROOT_DIR = ..
SHADER_COMPILE_ARGS = $$PWD/shaders shader_index.txt shader_def
include($$ROOT_DIR/common.pri)

DRAPE_DIR = .
include($$DRAPE_DIR/drape_common.pri)

SOURCES += glfunctions.cpp

OTHER_FILES += \
    shaders/simple_vertex_shader.vsh \
    shaders/solid_area_fragment_shader.fsh \
    shaders/texturing_vertex_shader.vsh \
    shaders/shader_index.txt \
    shaders/texturing_fragment_shader.fsh \
    shaders/line_vertex_shader.vsh \
    shaders/line_fragment_shader.fsh \
    shaders/solid_color_fragment_shader.fsh \
    shaders/normalize_vertex_shader.vsh
