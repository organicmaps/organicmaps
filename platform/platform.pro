# Platform independent interface to platform dependent code :)

TARGET = platform
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = coding base jansson

include($$ROOT_DIR/common.pri)

QT *= core network

!iphone* {
  INCLUDEPATH += $$ROOT_DIR/3party/jansson/src

  SOURCES += \
    qtplatform.cpp \
    wifi_location_service.cpp \
    qt_download_manager.cpp \
    qt_download.cpp \

  HEADERS += \
      qt_download_manager.hpp \
      qt_download.hpp \
}

HEADERS += \
    platform.hpp \
    download_manager.hpp \
    location.hpp \
    wifi_info.hpp

macx|iphone* {
    OBJECTIVE_SOURCES += apple_location_service.mm
    LIBS += -framework CoreLocation -framework Foundation
} else {
    SOURCES +=
}

macx:!iphone* {
  OBJECTIVE_SOURCES += wifi_info_mac.mm
  LIBS += -framework CoreWLAN
}
win32 {
  SOURCES += wifi_info_windows.cpp
}

# common sources for all platforms
SOURCES += \
    location_manager.cpp \
