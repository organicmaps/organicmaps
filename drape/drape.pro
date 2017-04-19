TARGET = drape
TEMPLATE = lib
CONFIG += staticlib warn_on
INCLUDEPATH += ../3party/icu/common ../3party/icu/i18n
DEFINES += U_DISABLE_RENAMING

ROOT_DIR = ..
SHADER_COMPILE_ARGS = $$PWD/shaders shader_index.txt shader_def
include($$ROOT_DIR/common.pri)

DRAPE_DIR = .
include($$DRAPE_DIR/drape_common.pri)

SOURCES += glfunctions.cpp

OTHER_FILES += \
    shaders/area3d_vertex_shader.vsh \
    shaders/area3d_outline_vertex_shader.vsh \
    shaders/area_vertex_shader.vsh \
    shaders/arrow3d_fragment_shader.fsh \
    shaders/arrow3d_outline_fragment_shader.fsh \
    shaders/arrow3d_shadow_fragment_shader.fsh \
    shaders/arrow3d_shadow_vertex_shader.vsh \
    shaders/arrow3d_vertex_shader.vsh \
    shaders/circle_shader.fsh \
    shaders/circle_shader.vsh \
    shaders/colored_symbol_billboard_shader.vsh \
    shaders/colored_symbol_shader.fsh \
    shaders/colored_symbol_shader.vsh \
    shaders/compass_vertex_shader.vsh \
    shaders/dashed_fragment_shader.fsh \
    shaders/dashed_vertex_shader.vsh \
    shaders/debug_rect_fragment_shader.fsh \
    shaders/debug_rect_vertex_shader.vsh \
    shaders/discarded_texturing_fragment_shader.fsh \
    shaders/hatching_area_fragment_shader.fsh \
    shaders/hatching_area_vertex_shader.vsh \
    shaders/line_fragment_shader.fsh \
    shaders/line_vertex_shader.vsh \
    shaders/masked_texturing_billboard_vertex_shader.vsh \
    shaders/masked_texturing_fragment_shader.fsh \
    shaders/masked_texturing_vertex_shader.vsh \
    shaders/my_position_shader.vsh \
    shaders/path_symbol_vertex_shader.vsh \
    shaders/position_accuracy3d_shader.vsh \
    shaders/route_arrow_vertex_shader.vsh \
    shaders/route_dash_fragment_shader.fsh \
    shaders/route_fragment_shader.fsh \
    shaders/route_vertex_shader.vsh \
    shaders/ruler_vertex_shader.vsh \
    shaders/shader_index.txt \
    shaders/solid_color_fragment_shader.fsh \
    shaders/text_billboard_vertex_shader.vsh \
    shaders/text_fixed_fragment_shader.fsh \
    shaders/text_fixed_vertex_shader.vsh \
    shaders/text_fragment_shader.fsh \
    shaders/text_outlined_billboard_vertex_shader.vsh \
    shaders/text_outlined_gui_vertex_shader.vsh \
    shaders/text_outlined_vertex_shader.vsh \
    shaders/text_vertex_shader.vsh \
    shaders/texturing3d_fragment_shader.fsh \
    shaders/texturing3d_vertex_shader.vsh \
    shaders/texturing_billboard_vertex_shader.vsh \
    shaders/texturing_fragment_shader.fsh \
    shaders/texturing_vertex_shader.vsh \
    shaders/trackpoint_vertex_shader.vsh \
    shaders/trackpoint_fragment_shader.fsh \
    shaders/traffic_fragment_shader.fsh \
    shaders/traffic_vertex_shader.vsh \
    shaders/traffic_line_fragment_shader.fsh \
    shaders/traffic_line_vertex_shader.vsh \
    shaders/transparent_layer_fragment_shader.fsh \
    shaders/transparent_layer_vertex_shader.vsh \
    shaders/user_mark.vsh \
    shaders/user_mark_billboard.vsh \
