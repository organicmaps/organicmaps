TARGET = sgitess
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

CONFIG -= warn_on
CONFIG *= warn_off

HEADERS += \
    tessmono.h \
    tess.h \
    sweep.h \
    priorityq.h \
    priorityq-sort.h \
    priorityq-heap.h \
    normal.h \
    mesh.h \
    memalloc.h \
    geom.h \
    dict.h \
    dict-list.h \
    render.h \
    gluos.h \
    GL/wmesa.h \
    GL/wglext.h \
    GL/vms_x_fix.h \
    GL/osmesa.h \
    GL/mglmesa.h \
    GL/mesa_wgl.h \
    GL/glxext.h \
    GL/glx.h \
    GL/glx_mangle.h \
    GL/glu.h \
    GL/glu_mangle.h \
    GL/glfbdev.h \
    GL/glext.h \
    GL/gl.h \
    GL/gl_mangle.h \
    GL/internal/sarea.h \
    GL/internal/glcore.h \
    GL/internal/dri_interface.h \
    interface.h

SOURCES += \
    tessmono.c \
    tess.c \
    sweep.c \
    priorityq.c \
    priorityq-heap.c \
    normal.c \
    mesh.c \
    memalloc.c \
    geom.c \
    dict.c \
    render.c \
    interface.cpp
