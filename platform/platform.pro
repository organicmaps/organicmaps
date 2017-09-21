# Platform independent interface to platform dependent code :)

TARGET = platform
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

INCLUDEPATH += $$ROOT_DIR/3party/jansson/src

!iphone*:!android*:!tizen {
  QT *= core

  SOURCES += location_service.cpp \
             marketing_service_dummy.cpp \
             platform_qt.cpp \
             wifi_location_service.cpp
  HEADERS += location_service.hpp \
             wifi_info.hpp
  !macx-* {
    QT *= network
    SOURCES += http_thread_qt.cpp
    HEADERS += http_thread_qt.hpp
  }

  win32* {
    SOURCES += platform_win.cpp \
               secure_storage_dummy.cpp \
               wifi_info_windows.cpp
  } else:macx-* {
    OBJECTIVE_SOURCES += apple_location_service.mm \
                         gui_thread_apple.mm \
                         platform_mac.mm \
                         secure_storage_qt.cpp
  } else:linux* {
    SOURCES += gui_thread_linux.cpp \
               platform_linux.cpp \
               secure_storage_qt.cpp
  }
} else:iphone* {
  OBJECTIVE_SOURCES += marketing_service_ios.mm \
                       platform_ios.mm \
                       gui_thread_apple.mm \
                       secure_storage_ios.mm
} else:android* {
  SOURCES += platform_android.cpp
} else:tizen* {
  HEADERS += http_thread_tizen.hpp \
             tizen_utils.hpp
  SOURCES += http_thread_tizen.cpp \
             marketing_service_dummy.cpp \
             platform_tizen.cpp \
             secure_storage_dummy.cpp \
             tizen_utils.cpp
}

macx-*|iphone* {
  HEADERS += http_thread_apple.h
  OBJECTIVE_SOURCES += \
    http_thread_apple.mm \
    http_client_apple.mm \
    socket_apple.mm \

  QMAKE_OBJECTIVE_CFLAGS += -fobjc-arc
}

linux*|win* {
  SOURCES += http_client_curl.cpp
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
    gui_thread.hpp \
    http_request.hpp \
    http_thread_callback.hpp \
    http_client.hpp \
    local_country_file.hpp \
    local_country_file_utils.hpp \
    location.hpp \
    marketing_service.hpp \
    measurement_utils.hpp \
    mwm_traits.hpp \
    mwm_version.hpp \
    network_policy.hpp \
    platform.hpp \
    preferred_languages.hpp \
    secure_storage.hpp \
    safe_callback.hpp \
    servers_list.hpp \
    settings.hpp \
    socket.hpp \
    string_storage_base.hpp \

SOURCES += \
    chunks_download_strategy.cpp \
    country_defines.cpp \
    country_file.cpp \
    file_logging.cpp \
    get_text_by_id.cpp \
    http_client.cpp \
    http_request.cpp \
    local_country_file.cpp \
    local_country_file_utils.cpp \
    marketing_service.cpp \
    measurement_utils.cpp \
    mwm_traits.cpp \
    mwm_version.cpp \
    platform.cpp \
    preferred_languages.cpp \
    servers_list.cpp \
    settings.cpp \
    string_storage_base.cpp \
