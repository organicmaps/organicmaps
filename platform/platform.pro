# Platform independent interface to platform dependent code :)

TARGET = platform
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
DEPENDENCIES = coding base jansson

include($$ROOT_DIR/common.pri)

!iphone*:!android*:!bada {
  QT *= core
  INCLUDEPATH += $$ROOT_DIR/3party/jansson/src

  SOURCES += platform_qt.cpp \
             wifi_location_service.cpp \
             qt_concurrent_runner.cpp \
             location_service.cpp
  HEADERS += wifi_info.hpp \
             location_service.hpp
  win32* {
    SOURCES += platform_win.cpp \
               wifi_info_windows.cpp
  } else:macx* {
    OBJECTIVE_SOURCES += platform_mac.mm \
                         wifi_info_mac.mm \
                         apple_video_timer.mm \
                         apple_location_service.mm
  } else:linux* {
    SOURCES += platform_linux.cpp
  }
} else:iphone* {
  OBJECTIVE_SOURCES += ios_video_timer.mm \
                       ios_concurrent_runner.mm \
                       platform_ios.mm
} else:android* {
  SOURCES += platform_android.cpp
}

macx*|iphone* {
  HEADERS += apple_download.h
  OBJECTIVE_SOURCES += apple_download.mm \
                       apple_download_manager.mm
}

win32*|linux* {
  QT *= network
  HEADERS += qt_download_manager.hpp \
             qt_download.hpp
  SOURCES += qt_download_manager.cpp \
             qt_download.cpp
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
    url_generator.hpp \

SOURCES += \
    preferred_languages.cpp \
    settings.cpp \
    video_timer.cpp \
    languages.cpp \
    url_generator.cpp \
