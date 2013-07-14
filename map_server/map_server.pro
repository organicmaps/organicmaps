#-------------------------------------------------
#
# Project created by QtCreator 2013-07-13T08:23:48
#
#-------------------------------------------------

QT       += core gui opengl
TARGET = map_server
CONFIG   += console warn_on
CONFIG   -= app_bundle
TEMPLATE = app

DEPENDENCIES = map gui search storage indexer graphics platform anim geometry coding base \
               map_server_utils bzip2 freetype expat fribidi tomcrypt jansson protobuf

ROOT_DIR = ..
include($$ROOT_DIR/common.pri)

INCLUDEPATH += $$ROOT_DIR/3party/jansson/src

macx* {
  LIBS *= "-framework Foundation" "-framework CoreWLAN" \
          "-framework QuartzCore" "-framework IOKit"
}

SOURCES += main.cpp \
    render_context.cpp

HEADERS += \
    render_context.hpp
