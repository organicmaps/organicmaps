TARGET = drape
TEMPLATE = lib
CONFIG += staticlib warn_on
INCLUDEPATH += ../3party/icu/common ../3party/icu/i18n
DEFINES += U_DISABLE_RENAMING

ROOT_DIR = ..
include($$ROOT_DIR/common.pri)

DRAPE_DIR = .
include($$DRAPE_DIR/drape_common.pri)

SOURCES += glfunctions.cpp
