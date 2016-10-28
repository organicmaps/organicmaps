TARGET = traffic
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

SOURCES += \
    speed_groups.cpp \
    traffic_info.cpp \

HEADERS += \
    speed_groups.hpp \
    traffic_info.hpp \
