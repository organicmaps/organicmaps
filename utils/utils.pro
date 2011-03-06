TARGET = utils
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = base coding coding_sloynik

include($$ROOT_DIR/common.pri)

HEADERS += \

SOURCES += \
  dummy_utils.cpp \

OTHER_FILES += \


