# Platform independent interface to platform dependent code :)

TARGET = platform
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

INCLUDEPATH += $$ROOT_DIR/3party/jansson/src

!iphone*:!android*:!tizen {
  QT *= core

  SOURCES += platform_qt.cpp \
             wifi_location_service.cpp \
             location_service.cpp
  HEADERS += wifi_info.hpp \
             location_service.hpp
  !macx-* {
    QT *= network
    SOURCES += http_thread_qt.cpp
    HEADERS += http_thread_qt.hpp
  }

  win32* {
    SOURCES += platform_win.cpp \
               wifi_info_windows.cpp
  } else:macx-* {
    OBJECTIVE_SOURCES += platform_mac.mm \
                         apple_video_timer.mm \
                         apple_location_service.mm
  } else:linux* {
    SOURCES += platform_linux.cpp
  }
} else:iphone* {
  OBJECTIVE_SOURCES += ios_video_timer.mm \
                       platform_ios.mm
} else:android* {
  SOURCES += platform_android.cpp \
             pthread_video_timer.cpp
} else:tizen* {
  HEADERS += tizen_utils.hpp \
    http_thread_tizen.hpp
  SOURCES += platform_tizen.cpp \
    tizen_utils.cpp \
    pthread_video_timer.cpp \
    http_thread_tizen.cpp \
}

macx-*|iphone* {
  HEADERS += http_thread_apple.h
  OBJECTIVE_SOURCES += http_thread_apple.mm
}

!win32* {
  HEADERS += platform_unix_impl.hpp
  SOURCES += platform_unix_impl.cpp
}

# common sources for all platforms

HEADERS += \
    platform.hpp \
    location.hpp \
    preferred_languages.hpp \
    settings.hpp \
    video_timer.hpp \
    http_request.hpp \
    http_thread_callback.hpp \
    chunks_download_strategy.hpp \
    servers_list.hpp \
    constants.hpp \
    file_logging.hpp \

SOURCES += \
    preferred_languages.cpp \
    settings.cpp \
    video_timer.cpp \
    http_request.cpp \
    chunks_download_strategy.cpp \
    platform.cpp \
    servers_list.cpp \
    file_logging.cpp \
