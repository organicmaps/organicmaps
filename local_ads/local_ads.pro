TARGET = local_ads
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

SOURCES += \
    campaign_serialization.cpp \
    event.cpp \
    file_helpers.cpp \
    icons_info.cpp \
    statistics.cpp \


HEADERS += \
    campaign.hpp \
    campaign_serialization.hpp \
    event.hpp \
    file_helpers.hpp \
    icons_info.hpp \
    statistics.hpp \
