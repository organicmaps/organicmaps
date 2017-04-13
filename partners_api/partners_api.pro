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
    facebook_ads.cpp \
    mopub_ads.cpp \
    opentable_api.cpp \
    rb_ads.cpp \
    uber_api.cpp \

HEADERS += \
    ads_base.hpp \
    ads_engine.hpp \
    banner.hpp \
    booking_api.hpp \
    facebook_ads.hpp \
    mopub_ads.hpp \
    opentable_api.hpp \
    rb_ads.hpp \
    uber_api.hpp \
