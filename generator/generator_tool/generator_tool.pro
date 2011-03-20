# Generator binary

ROOT_DIR = ../..
DEPENDENCIES = map generator storage indexer platform geometry coding base gflags expat sgitess version

include($$ROOT_DIR/common.pri)

CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

# needed for Platform::WorkingDir()
QT += core

win32:LIBS += -lShell32

SOURCES += \
    generator_tool.cpp \

HEADERS += \
