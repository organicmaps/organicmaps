# Statistics client library.

TARGET = stats_client
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)
INCLUDEPATH += $$ROOT_DIR/3party/protobuf/src

DEPENDENCIES = base protobuf

SOURCES += \
    stats_client.cpp \
    stats_writer.cpp \
    ../common/wire.pb.cc \

HEADERS += \
    stats_client.hpp \
    stats_writer.hpp \
    ../common/wire.pb.h \

OTHER_FILES += ../common/wire.proto
