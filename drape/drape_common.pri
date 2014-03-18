CMDRES = $$system(python ../tools/autobuild/shader_preprocessor.py $$SHADER_COMPILE_ARGS)
message($$CMDRES)

SOURCES += \
    $$DRAPE_DIR/data_buffer.cpp \
    $$DRAPE_DIR/binding_info.cpp \
    $$DRAPE_DIR/batcher.cpp \
    $$DRAPE_DIR/attribute_provider.cpp \
    $$DRAPE_DIR/vertex_array_buffer.cpp \
    $$DRAPE_DIR/uniform_value.cpp \
    $$DRAPE_DIR/texture.cpp \
    $$DRAPE_DIR/shader.cpp \
    $$DRAPE_DIR/index_buffer.cpp \
    $$DRAPE_DIR/gpu_program.cpp \
    $$DRAPE_DIR/gpu_program_manager.cpp \
    $$DRAPE_DIR/glconstants.cpp \
    $$DRAPE_DIR/glstate.cpp \
    $$DRAPE_DIR/gpu_buffer.cpp \
    $$DRAPE_DIR/utils/list_generator.cpp \
    $$DRAPE_DIR/shader_def.cpp \
    $$DRAPE_DIR/glextensions_list.cpp \
    $$DRAPE_DIR/pointers.cpp \
    $$DRAPE_DIR/uniform_values_storage.cpp \
    $$DRAPE_DIR/color.cpp \
    $$DRAPE_DIR/oglcontextfactory.cpp \
    $$DRAPE_DIR/buffer_base.cpp \
    $$DRAPE_DIR/cpu_buffer.cpp \
    $$DRAPE_DIR/utils/lodepng.cpp \
    $$DRAPE_DIR/texture_structure_desc.cpp \
    $$DRAPE_DIR/symbols_texture.cpp

HEADERS += \
    $$DRAPE_DIR/data_buffer.hpp \
    $$DRAPE_DIR/binding_info.hpp \
    $$DRAPE_DIR/batcher.hpp \
    $$DRAPE_DIR/attribute_provider.hpp \
    $$DRAPE_DIR/vertex_array_buffer.hpp \
    $$DRAPE_DIR/uniform_value.hpp \
    $$DRAPE_DIR/texture.hpp \
    $$DRAPE_DIR/shader.hpp \
    $$DRAPE_DIR/pointers.hpp \
    $$DRAPE_DIR/index_buffer.hpp \
    $$DRAPE_DIR/gpu_program.hpp \
    $$DRAPE_DIR/gpu_program_manager.hpp \
    $$DRAPE_DIR/glstate.hpp \
    $$DRAPE_DIR/glIncludes.hpp \
    $$DRAPE_DIR/glconstants.hpp \
    $$DRAPE_DIR/glfunctions.hpp \
    $$DRAPE_DIR/gpu_buffer.hpp \
    $$DRAPE_DIR/utils/list_generator.hpp \
    $$DRAPE_DIR/shader_def.hpp \
    $$DRAPE_DIR/glextensions_list.hpp \
    $$DRAPE_DIR/oglcontext.hpp \
    $$DRAPE_DIR/uniform_values_storage.hpp \
    $$DRAPE_DIR/color.hpp \
    $$DRAPE_DIR/oglcontextfactory.hpp \
    $$DRAPE_DIR/buffer_base.hpp \
    $$DRAPE_DIR/cpu_buffer.hpp \
    $$DRAPE_DIR/utils/lodepng.h \
    $$DRAPE_DIR/texture_structure_desc.hpp \
    $$DRAPE_DIR/symbols_texture.hpp
