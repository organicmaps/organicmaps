# Platform independent interface to platform dependent code :)

TARGET = platform
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = base tomcrypt

include($$ROOT_DIR/common.pri)

INCLUDEPATH += ../3party/tomcrypt/src/headers

QT *= core network

SOURCES += \
  qtplatform.cpp \
  qt_download_manager.cpp \
  qt_download.cpp \

HEADERS += \
  platform.hpp \
  download_manager.hpp \
  qt_download_manager.hpp \
  qt_download.hpp \
