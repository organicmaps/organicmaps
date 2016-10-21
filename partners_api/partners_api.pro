TARGET = partners_api
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

INCLUDEPATH *= $$ROOT_DIR/3party/jansson/src

include($$ROOT_DIR/common.pri)

SOURCES += \
    booking_api.cpp \
    opentable_api.cpp \
    uber_api.cpp \

HEADERS += \
    booking_api.hpp \
    opentable_api.hpp \
    uber_api.hpp \
