# Statistics client library.

TARGET = stats_client
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)
INCLUDEPATH += $$ROOT_DIR/3party/protobuf/src

DEPENDENCIES = base protobuf

SOURCES += \
    ../common/wire.pb.cc \
    event_tracker.cpp \
    event_writer.cpp

HEADERS += \
    ../common/wire.pb.h \
    event_tracker.hpp \
    event_writer.hpp

OTHER_FILES += ../common/wire.proto
