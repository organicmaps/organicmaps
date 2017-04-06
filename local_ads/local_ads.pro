TARGET = local_ads
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

SOURCES += \
    campaign_serialization.cpp \
    local_ads_helpers.cpp \


HEADERS += \
    campaign.hpp \
    campaign_serialization.hpp \
    local_ads_helpers.hpp \
