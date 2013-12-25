# Head project for drape develop and debuging
ROOT_DIR = ..
DEPENDENCIES = drape base

include($$ROOT_DIR/common.pri)

TARGET = DrapeHead
TEMPLATE = app
CONFIG += warn_on
QT *= core widgets gui

win32* {
  LIBS += -lopengl32 -lws2_32 -lshell32 -liphlpapi
  RC_FILE = res/windows.rc
  win32-msvc*: LIBS += -lwlanapi
  win32-g++: LIBS += -lpthread
}

win32*|linux* {
  QT *= network
}

HEADERS += \
    mainwindow.hpp \
    glwidget.hpp \
    qtoglcontext.hpp \
    qtoglcontextfactory.hpp \
    drape_surface.hpp \

SOURCES += \
    mainwindow.cpp \
    main.cpp \
    glwidget.cpp \
    qtoglcontext.cpp \
    qtoglcontextfactory.cpp \ 
    drape_surface.cpp \
