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
                         apple_location_service.mm
  } else:linux* {
    SOURCES += platform_linux.cpp
  }
} else:iphone* {
  OBJECTIVE_SOURCES += platform_ios.mm
} else:android* {
  SOURCES += platform_android.cpp \
} else:tizen* {
  HEADERS += tizen_utils.hpp \
    http_thread_tizen.hpp
  SOURCES += platform_tizen.cpp \
    tizen_utils.cpp \
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
    chunks_download_strategy.hpp \
    constants.hpp \
    country_defines.hpp \
    country_file.hpp \
    file_logging.hpp \
    get_text_by_id.hpp \
    http_request.hpp \
    http_thread_callback.hpp \
    local_country_file.hpp \
    local_country_file_utils.hpp \
    location.hpp \
    measurement_utils.hpp \
    mwm_version.hpp \
    platform.hpp \
    preferred_languages.hpp \
    servers_list.hpp \
    settings.hpp \

SOURCES += \
    chunks_download_strategy.cpp \
    country_defines.cpp \
    country_file.cpp \
    file_logging.cpp \
    get_text_by_id.cpp \
    http_request.cpp \
    local_country_file.cpp \
    local_country_file_utils.cpp \
    measurement_utils.cpp \
    mwm_version.cpp \
    platform.cpp \
    preferred_languages.cpp \
    servers_list.cpp \
    settings.cpp \
