# Head project for drape develop and debuging
ROOT_DIR = ..
DEPENDENCIES = drape_frontend map drape indexer platform geometry coding base expat protobuf jansson zlib

include($$ROOT_DIR/common.pri)

INCLUDEPATH += $$ROOT_DIR/3party/jansson/src

TARGET = DrapeHead
TEMPLATE = app
CONFIG += warn_on
QT *= core gui widgets
#DEFINES += USE_TESTING_ENGINE

win32* {
  LIBS += -lopengl32 -lws2_32 -lshell32 -liphlpapi
  RC_FILE = res/windows.rc
  win32-msvc*: LIBS += -lwlanapi
  win32-g++: LIBS += -lpthread
}

win32*|linux* {
  QT *= network
}

macx-* {
  LIBS *= "-framework CoreLocation" "-framework Foundation" "-framework CoreWLAN" \
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
