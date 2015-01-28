# Base functions project.
TARGET = routing
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

DEFINES *= BOOST_ERROR_CODE_HEADER_ONLY
INCLUDEPATH += $$ROOT_DIR/3party/jansson/src \
               $$ROOT_DIR/3party/osrm/osrm-backend/Include

SOURCES += \
    turns.cpp \
    route.cpp \
    osrm_router.cpp \
    osrm_online_router.cpp \
    osrm2feature_map.cpp \
    vehicle_model.cpp \
    ../3party/succinct/rs_bit_vector.cpp \

HEADERS += \
    turns.hpp \
    route.hpp \
    router.hpp \
    osrm_router.hpp \
    osrm_online_router.hpp \
    osrm_data_facade.hpp \
    osrm2feature_map.hpp \
    vehicle_model.hpp \
    cross_routing_context.hpp
