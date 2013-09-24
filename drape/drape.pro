#-------------------------------------------------
#
# Project created by QtCreator 2013-09-17T15:41:48
#
#-------------------------------------------------

TARGET = drape
TEMPLATE = lib
CONFIG += staticlib warn_on

DEPENDENCIES = base

ROOT_DIR = ..
include($$ROOT_DIR/common.pri)

DRAPE_DIR = .
include($$DRAPE_DIR/drape_common.pri)

SOURCES += glfunctions.cpp
