# Platform independent interface to platform dependent code :)

TARGET = platform
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = coding base

include($$ROOT_DIR/common.pri)

QT *= core

!iphone* {
  SOURCES += \
    qtplatform.cpp

  CONFIG(debug, debug|release) {
    QT *= network
    SOURCES += \
      qt_download_manager.cpp \
      qt_download.cpp
  }

  HEADERS += \
      qt_download_manager.hpp \
      qt_download.hpp \
}

HEADERS += \
    platform.hpp \
    download_manager.hpp \
    location.hpp \

mac|iphone* {
    OBJECTIVE_SOURCES += apple_location_service.mm
    LIBS += -framework CoreLocation -framework Foundation
} else {
    SOURCES += qt_location_service.cpp
}
