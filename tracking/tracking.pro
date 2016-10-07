TARGET = tracking
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

SOURCES += \
    connection.cpp \
    reporter.cpp \

HEADERS += \
    connection.hpp \
    reporter.hpp \
