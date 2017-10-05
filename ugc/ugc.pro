# UGC library.

TARGET = ugc
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

HEADERS += \
    api.hpp \
    binary/header_v0.hpp \
    binary/index_ugc.hpp \
    binary/serdes.hpp \
    binary/ugc_holder.hpp \
    binary/visitors.hpp \
    loader.hpp \
    serdes.hpp \
    serdes_json.hpp \
    storage.hpp \
    types.hpp \

SOURCES += \
    api.cpp \
    binary/serdes.cpp \
    loader.cpp \
    storage.cpp \
