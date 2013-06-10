TARGET = stats_client_tests
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../../..
DEPENDENCIES = stats_client coding base protobuf leveldb cityhash
include($$ROOT_DIR/common.pri)
INCLUDEPATH += $$ROOT_DIR/3party/leveldb/include \
               $$ROOT_DIR/3party/protobuf/src \
               $$ROOT_DIR/3party/cityhash/src

QT *= core

HEADERS += \
    ../leveldb_reader.hpp

SOURCES += \
    $$ROOT_DIR/testing/testingmain.cpp \
    stats_writer_test.cpp \
