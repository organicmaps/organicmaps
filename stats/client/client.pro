# Statistics client library.

TARGET = stats_client
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)
INCLUDEPATH += $$ROOT_DIR/3party/leveldb/include \
               $$ROOT_DIR/3party/protobuf/src \
               $$ROOT_DIR/3party/cityhash/src \


DEPENDENCIES = base protobuf leveldb cityhash


SOURCES += \
    stats_client.cpp \
    stats_writer.cpp \
    ../common/wire.pb.cc \

HEADERS += \
    stats_client.hpp \
    stats_writer.hpp \
    leveldb_reader.hpp \
    ../common/wire.pb.h \

OTHER_FILES += ../common/wire.proto
