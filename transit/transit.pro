# A higher level routing project used to avoid cyclic dependencies.
TARGET = transit
TEMPLATE = lib
CONFIG += staticlib warn_on c++11

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

DEFINES *= BOOST_ERROR_CODE_HEADER_ONLY
INCLUDEPATH += $$ROOT_DIR/3party/jansson/src

SOURCES += \
    transit_graph_data.cpp \
    transit_types.cpp \

HEADERS += \
    transit_graph_data.hpp \
    transit_serdes.hpp \
    transit_speed_limits.hpp \
    transit_types.hpp \
