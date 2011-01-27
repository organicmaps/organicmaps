# Platform independent interface to platform dependent code :)

TARGET = platform
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = base version

include($$ROOT_DIR/common.pri)

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
