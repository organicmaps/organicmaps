TARGET = drape
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..
SHADER_COMPILE_ARGS = $$PWD/shaders shader_index.txt shader_def
include($$ROOT_DIR/common.pri)

DRAPE_DIR = .
include($$DRAPE_DIR/drape_common.pri)

SOURCES += glfunctions.cpp

OTHER_FILES += \
    shaders/compass_vertex_shader.vsh \
    shaders/dashed_fragment_shader.fsh \
    shaders/dashed_vertex_shader.vsh \
    shaders/line_fragment_shader.fsh \
    shaders/line_vertex_shader.vsh \
    shaders/my_position_shader.vsh \
    shaders/position_accuracy_shader.vsh \
    shaders/ruler_vertex_shader.vsh \
    shaders/shader_index.txt \
    shaders/text_fragment_shader.fsh \
    shaders/text_vertex_shader.vsh \
    shaders/texturing_fragment_shader.fsh \
    shaders/texturing_vertex_shader.vsh \
    shaders/user_mark.vsh \
    shaders/circle_shader.fsh \
    shaders/circle_shader.vsh \
    shaders/area_vertex_shader.vsh \
