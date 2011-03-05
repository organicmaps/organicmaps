TARGET = utils
TEMPLATE = lib
CONFIG += staticlib

SLOYNIK_DIR = ..
DEPENDENCIES = base coding coding_sloynik

include($$SLOYNIK_DIR/sloynik_common.pri)

HEADERS += \

SOURCES += \
  dummy_utils.cpp \

OTHER_FILES += \


