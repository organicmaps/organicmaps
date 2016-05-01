# Map library tests.

TARGET = benchmark_tool
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..

DEPENDENCIES = map indexer platform editor geometry coding base gflags protobuf tomcrypt succinct pugixml stats_client

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/gflags/src

QT *= core

SOURCES += \
    features_loading.cpp \
    main.cpp \
    api.cpp \

HEADERS += \
    api.hpp \
