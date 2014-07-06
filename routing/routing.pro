# Base functions project.
TARGET = routing
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

SOURCES += \
    route.cpp \
    routing_engine.cpp \
    road_graph.cpp \

HEADERS += \
    route.hpp \
    routing_engine.hpp \
    router.hpp \
    road_graph.hpp \
