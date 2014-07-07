TARGET = MapsWithMe-server
CONFIG   += console warn_on
CONFIG   -= app_bundle
TEMPLATE = app

DEFINES += QJSONRPC_BUILD

DEPENDENCIES = map gui routing search storage indexer graphics platform anim geometry coding base \
               bzip2 freetype expat fribidi tomcrypt jansson protobuf qjsonrpc gflags

ROOT_DIR = ..
include($$ROOT_DIR/common.pri)

target.path = /usr/bin
INSTALLS += target

QT       += core gui opengl network

INCLUDEPATH *= $$ROOT_DIR/3party/qjsonrpc/src
INCLUDEPATH *= $$ROOT_DIR/3party/gflags/src

macx* {
  LIBS *= "-framework Foundation" "-framework CoreWLAN" \
          "-framework QuartzCore" "-framework IOKit"
}

SOURCES += main.cpp \
    render_context.cpp \

HEADERS += \
    main.hpp \
    render_context.hpp \
