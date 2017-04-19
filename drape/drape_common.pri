CMDRES = $$system(python ../tools/autobuild/shader_preprocessor.py $$SHADER_COMPILE_ARGS)
!isEmpty($$CMDRES):message($$CMDRES)

INCLUDEPATH *= $$ROOT_DIR/3party/freetype/include

SOURCES += \
    $$DRAPE_DIR/attribute_buffer_mutator.cpp \
    $$DRAPE_DIR/attribute_provider.cpp \
    $$DRAPE_DIR/batcher.cpp \
    $$DRAPE_DIR/batcher_helpers.cpp \
    $$DRAPE_DIR/bidi.cpp \
    $$DRAPE_DIR/binding_info.cpp \
    $$DRAPE_DIR/buffer_base.cpp \
    $$DRAPE_DIR/color.cpp \
    $$DRAPE_DIR/cpu_buffer.cpp \
    $$DRAPE_DIR/data_buffer.cpp \
    $$DRAPE_DIR/debug_rect_renderer.cpp \
    $$DRAPE_DIR/font_texture.cpp \
    $$DRAPE_DIR/glconstants.cpp \
    $$DRAPE_DIR/glextensions_list.cpp \
    $$DRAPE_DIR/glstate.cpp \
    $$DRAPE_DIR/glyph_manager.cpp \
    $$DRAPE_DIR/gpu_buffer.cpp \
    $$DRAPE_DIR/gpu_program.cpp \
    $$DRAPE_DIR/gpu_program_manager.cpp \
    $$DRAPE_DIR/hw_texture.cpp \
    $$DRAPE_DIR/index_buffer.cpp \
    $$DRAPE_DIR/index_buffer_mutator.cpp \
    $$DRAPE_DIR/index_storage.cpp \
    $$DRAPE_DIR/oglcontextfactory.cpp \
    $$DRAPE_DIR/overlay_handle.cpp \
    $$DRAPE_DIR/overlay_tree.cpp \
    $$DRAPE_DIR/pointers.cpp \
    $$DRAPE_DIR/render_bucket.cpp \
    $$DRAPE_DIR/shader.cpp \
    $$DRAPE_DIR/shader_def.cpp \
    $$DRAPE_DIR/static_texture.cpp \
    $$DRAPE_DIR/stipple_pen_resource.cpp \
    $$DRAPE_DIR/support_manager.cpp \
    $$DRAPE_DIR/symbols_texture.cpp \
    $$DRAPE_DIR/texture.cpp \
    $$DRAPE_DIR/texture_manager.cpp \
    $$DRAPE_DIR/texture_of_colors.cpp \
    $$DRAPE_DIR/uniform_value.cpp \
    $$DRAPE_DIR/uniform_values_storage.cpp \
    $$DRAPE_DIR/utils/glyph_usage_tracker.cpp \
    $$DRAPE_DIR/utils/gpu_mem_tracker.cpp \
    $$DRAPE_DIR/utils/projection.cpp \
    $$DRAPE_DIR/utils/vertex_decl.cpp \
    $$DRAPE_DIR/vertex_array_buffer.cpp \

HEADERS += \
    $$DRAPE_DIR/attribute_buffer_mutator.hpp \
    $$DRAPE_DIR/attribute_provider.hpp \
    $$DRAPE_DIR/batcher.hpp \
    $$DRAPE_DIR/batcher_helpers.hpp \
    $$DRAPE_DIR/bidi.hpp \
    $$DRAPE_DIR/binding_info.hpp \
    $$DRAPE_DIR/buffer_base.hpp \
    $$DRAPE_DIR/color.hpp \
    $$DRAPE_DIR/constants.hpp \
    $$DRAPE_DIR/cpu_buffer.hpp \
    $$DRAPE_DIR/data_buffer.hpp \
    $$DRAPE_DIR/data_buffer_impl.hpp \
    $$DRAPE_DIR/debug_rect_renderer.hpp \
    $$DRAPE_DIR/drape_diagnostics.hpp \
    $$DRAPE_DIR/drape_global.hpp \
    $$DRAPE_DIR/dynamic_texture.hpp \
    $$DRAPE_DIR/font_texture.hpp \
    $$DRAPE_DIR/glconstants.hpp \
    $$DRAPE_DIR/glextensions_list.hpp \
    $$DRAPE_DIR/glfunctions.hpp \
    $$DRAPE_DIR/glIncludes.hpp \
    $$DRAPE_DIR/glsl_func.hpp \
    $$DRAPE_DIR/glsl_types.hpp \
    $$DRAPE_DIR/glstate.hpp \
    $$DRAPE_DIR/glyph_manager.hpp \
    $$DRAPE_DIR/gpu_buffer.hpp \
    $$DRAPE_DIR/gpu_program.hpp \
    $$DRAPE_DIR/gpu_program_manager.hpp \
    $$DRAPE_DIR/hw_texture.hpp \
    $$DRAPE_DIR/index_buffer.hpp \
    $$DRAPE_DIR/index_buffer_mutator.hpp \
    $$DRAPE_DIR/index_storage.hpp \
    $$DRAPE_DIR/object_pool.hpp \
    $$DRAPE_DIR/oglcontext.hpp \
    $$DRAPE_DIR/oglcontextfactory.hpp \
    $$DRAPE_DIR/overlay_handle.hpp \
    $$DRAPE_DIR/overlay_tree.hpp \
    $$DRAPE_DIR/pointers.hpp \
    $$DRAPE_DIR/render_bucket.hpp \
    $$DRAPE_DIR/shader.hpp \
    $$DRAPE_DIR/shader_def.hpp \
    $$DRAPE_DIR/static_texture.hpp \
    $$DRAPE_DIR/stipple_pen_resource.hpp \
    $$DRAPE_DIR/support_manager.hpp \
    $$DRAPE_DIR/symbols_texture.hpp \
    $$DRAPE_DIR/texture.hpp \
    $$DRAPE_DIR/texture_manager.hpp \
    $$DRAPE_DIR/texture_of_colors.hpp \
    $$DRAPE_DIR/uniform_value.hpp \
    $$DRAPE_DIR/uniform_values_storage.hpp \
    $$DRAPE_DIR/utils/glyph_usage_tracker.hpp \
    $$DRAPE_DIR/utils/gpu_mem_tracker.hpp \
    $$DRAPE_DIR/utils/projection.hpp \
    $$DRAPE_DIR/utils/vertex_decl.hpp \
    $$DRAPE_DIR/vertex_array_buffer.hpp \
    $$DRAPE_DIR/visual_scale.hpp \

iphone*{
    HEADERS += $$DRAPE_DIR/hw_texture_ios.hpp
    OBJECTIVE_SOURCES += $$DRAPE_DIR/hw_texture_ios.mm
}
