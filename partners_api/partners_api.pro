TARGET = partners_api
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

INCLUDEPATH *= $$ROOT_DIR/3party/jansson/src

include($$ROOT_DIR/common.pri)

SOURCES += \
    ads_base.cpp \
    ads_engine.cpp \
    booking_api.cpp \
    booking_availability_params.cpp \
    cian_api.cpp \
    facebook_ads.cpp \
    google_ads.cpp \
    locals_api.cpp \
    mopub_ads.cpp \
    opentable_api.cpp \
    rb_ads.cpp \
    taxi_base.cpp \
    taxi_countries.cpp \
    taxi_engine.cpp \
    uber_api.cpp \
    utils.cpp \
    viator_api.cpp \
    yandex_api.cpp \

HEADERS += \
    ads_base.hpp \
    ads_engine.hpp \
    banner.hpp \
    booking_api.hpp \
    booking_availability_params.hpp
    cian_api.hpp \
    facebook_ads.hpp \
    google_ads.hpp \
    locals_api.hpp \
    mopub_ads.hpp \
    opentable_api.hpp \
    rb_ads.hpp \
    taxi_base.hpp \
    taxi_countries.hpp \
    taxi_engine.hpp \
    taxi_places.hpp \
    taxi_provider.hpp \
    uber_api.hpp \
    utils.hpp \
    viator_api.hpp \
    yandex_api.hpp \
