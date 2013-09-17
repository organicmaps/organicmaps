#-------------------------------------------------
#
# Project created by QtCreator 2013-09-17T15:41:48
#
#-------------------------------------------------

TARGET = drape
TEMPLATE = lib
CONFIG += staticlib warn_on
DEFINES += DRAPE_ENGINE

DEPENDENCIES = base

ROOT_DIR = ..
include($$ROOT_DIR/common.pri)

#INCLUDEPATH += $$ROOT_DIR/3party/freetype/include $$ROOT_DIR/3party/agg

SOURCES += \
    data_buffer.cpp \
    binding_info.cpp \
    batcher.cpp \
    attribute_provider.cpp \
    vertex_array_buffer.cpp \
    uniform_value.cpp \
    texture.cpp \
    shader_reference.cpp \
    index_buffer.cpp \
    gpu_program.cpp \
    gpu_program_manager.cpp \
    glstate.cpp \
    glIncludes.cpp \
    glfunctions.cpp \
    glbuffer.cpp

HEADERS += \
    data_buffer.hpp \
    binding_info.hpp \
    batcher.hpp \
    attribute_provider.hpp \
    vertex_array_buffer.hpp \
    uniform_value.hpp \
    texture.hpp \
    shader_reference.hpp \
    pointers.hpp \
    index_buffer.hpp \
    gpu_program.hpp \
    gpu_program_manager.hpp \
    glstate.hpp \
    glIncludes.hpp \
    glfunctions.hpp \
    glbuffer.hpp
