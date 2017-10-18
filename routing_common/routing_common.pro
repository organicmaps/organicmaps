# A higher level routing project used to avoid cyclic dependencies.
TARGET = routing_common
TEMPLATE = lib
CONFIG += staticlib warn_on c++11

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

DEFINES *= BOOST_ERROR_CODE_HEADER_ONLY
INCLUDEPATH += $$ROOT_DIR/3party/jansson/src \
               $$ROOT_DIR/3party/osrm/osrm-backend/include \
               $$ROOT_DIR/3party/osrm/osrm-backend/third_party

SOURCES += \
    bicycle_model.cpp \
    car_model.cpp \
    pedestrian_model.cpp \
    transit_types.cpp \
    vehicle_model.cpp \


HEADERS += \
    bicycle_model.hpp \
    car_model.hpp \
    num_mwm_id.hpp \
    pedestrian_model.hpp \
    transit_serdes.hpp \
    transit_speed_limits.hpp \
    transit_types.hpp \
    vehicle_model.hpp \
