#-------------------------------------------------
#
# Project created by QtCreator 2013-07-13T21:44:19
#
#-------------------------------------------------
TARGET = map_server_utils
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..
include($$ROOT_DIR/common.pri)

INCLUDEPATH += $$ROOT_DIR/3party/jansson/src

DEPENDENCIES = platform jansson

SOURCES += \
    viewport.cpp \
    response.cpp \
    request.cpp \
    response_impl/response_cout.cpp

HEADERS += \
    viewport.hpp \
    response.hpp \
    request.hpp \
    response_impl/response_cout.hpp
