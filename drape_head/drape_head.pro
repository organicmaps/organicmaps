# Head project for drape develop and debuging
ROOT_DIR = ..
DEPENDENCIES = map drape_frontend drape_gui anim drape indexer storage platform geometry coding base \
               freetype expat protobuf jansson fribidi tomcrypt

include($$ROOT_DIR/common.pri)

INCLUDEPATH += $$ROOT_DIR/3party/jansson/src

TARGET = DrapeHead
TEMPLATE = app
CONFIG += warn_on
QT *= core gui widgets opengl

win32* {
  LIBS += -lopengl32 -lws2_32 -liphlpapi
#  RC_FILE = res/windows.rc
  win32-msvc*: LIBS += -lwlanapi
}

win32*|linux* {
  QT *= network
}

macx-* {
  LIBS *= "-framework CoreLocation" "-framework CoreWLAN" \
          "-framework QuartzCore" "-framework IOKit"
}

HEADERS += \
    mainwindow.hpp \
    qtoglcontext.hpp \
    qtoglcontextfactory.hpp \
    drape_surface.hpp \
    testing_engine.hpp \

SOURCES += \
    mainwindow.cpp \
    main.cpp \
    qtoglcontext.cpp \
    qtoglcontextfactory.cpp \
    drape_surface.cpp \
    testing_engine.cpp \
