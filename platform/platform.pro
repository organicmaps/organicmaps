# Platform independent interface to platform dependent code :)

TARGET = platform
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = coding base jansson

include($$ROOT_DIR/common.pri)

QT *= core network

!iphone*:!android*:!bada {
  INCLUDEPATH += $$ROOT_DIR/3party/jansson/src

  SOURCES += platform_qt.cpp \
             wifi_location_service.cpp \
             qt_download_manager.cpp \
             qt_download.cpp \
             qt_concurrent_runner.cpp
  HEADERS += qt_download_manager.hpp \
             qt_download.hpp \
             wifi_info.hpp
  win32* {
    SOURCES += platform_win.cpp \
               wifi_info_windows.cpp
  } else:macx* {
    OBJECTIVE_SOURCES += platform_mac.mm \
                         wifi_info_mac.mm \
                         apple_video_timer.mm
  } else:linux* {
    SOURCES += platform_linux.cpp
  }
} else:iphone* {
  OBJECTIVE_SOURCES += ios_video_timer.mm \
                       ios_concurrent_runner.mm \
                       platform_ios.mm
}

macx|iphone* {
  OBJECTIVE_SOURCES += apple_location_service.mm
}

# common sources for all platforms

HEADERS += \
    platform.hpp \
    download_manager.hpp \
    location.hpp \
    concurrent_runner.hpp \
    preferred_languages.hpp \
    settings.hpp \
    video_timer.hpp \
    languages.hpp \

SOURCES += \
    location_manager.cpp \
    preferred_languages.cpp \
    settings.cpp \
    video_timer.cpp \
    languages.cpp \
