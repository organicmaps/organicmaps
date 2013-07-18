TARGET = map_server
CONFIG   += console warn_on
CONFIG   -= app_bundle
TEMPLATE = app

DEPENDENCIES = map gui search storage indexer graphics platform anim geometry coding base \
               bzip2 freetype expat fribidi tomcrypt jansson protobuf

ROOT_DIR = ..
include($$ROOT_DIR/common.pri)

QT       += core gui opengl network

INCLUDEPATH += $$ROOT_DIR/3party/jansson/src

macx* {
  LIBS *= "-framework Foundation" "-framework CoreWLAN" \
          "-framework QuartzCore" "-framework IOKit"
}

SOURCES += main.cpp \
    render_context.cpp \
    request.cpp \
    response.cpp \
    response_impl/response_cout.cpp \
    viewport.cpp \

HEADERS += \
    render_context.hpp \
    request.hpp \
    response.hpp \
    viewport.hpp \
    response_impl/response_cout.hpp \

