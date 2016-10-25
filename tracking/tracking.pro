TARGET = tracking
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

SOURCES += \
    connection.cpp \
    protocol.cpp \
    reporter.cpp \

HEADERS += \
    connection.hpp \
    protocol.hpp \
    reporter.hpp \
