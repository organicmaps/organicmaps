# Base functions project.
TARGET = routing
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

INCLUDEPATH += $$ROOT_DIR/3party/jansson/src

SOURCES += \
    route.cpp \
    routing_engine.cpp \
    road_graph.cpp \
    helicopter_router.cpp \
    osrm_router.cpp \

HEADERS += \
    route.hpp \
    routing_engine.hpp \
    router.hpp \
    road_graph.hpp \
    helicopter_router.hpp \
    osrm_router.hpp \
