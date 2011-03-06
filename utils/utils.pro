TARGET = utils
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = base coding

include($$ROOT_DIR/common.pri)

HEADERS += \

SOURCES += \
  dummy_utils.cpp \

OTHER_FILES += \


