# Statistics client library.

TARGET = stats_client
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..
include($$ROOT_DIR/common.pri)

SOURCES += $$ROOT_DIR/3party/Alohalytics/src/cpp/alohalytics.cc \

HEADERS += $$ROOT_DIR/3party/Alohalytics/src/alohalytics.h \
           $$ROOT_DIR/3party/Alohalytics/src/event_base.h \
           $$ROOT_DIR/3party/Alohalytics/src/file_manager.h \
           $$ROOT_DIR/3party/Alohalytics/src/http_client.h \
           $$ROOT_DIR/3party/Alohalytics/src/logger.h \

win* {
  SOURCES += $$ROOT_DIR/3party/Alohalytics/src/windows/file_manager_windows_impl.cc
} else {
  SOURCES += $$ROOT_DIR/3party/Alohalytics/src/posix/file_manager_posix_impl.cc
}

macx-*|iphone* {
  HEADERS += $$ROOT_DIR/3party/Alohalytics/src/alohalytics_objc.h
  OBJECTIVE_SOURCES += $$ROOT_DIR/3party/Alohalytics/src/apple/http_client_apple.mm \
                       $$ROOT_DIR/3party/Alohalytics/src/apple/alohalytics_objc.mm
  QMAKE_OBJECTIVE_CFLAGS += -fobjc-arc
} else:linux*|win* {
  SOURCES += $$ROOT_DIR/3party/Alohalytics/src/posix/http_client_curl.cc
} else:android* {
  SOURCES += $$ROOT_DIR/3party/Alohalytics/src/android/jni/jni_alohalytics.cc
}
 # Android impl is included in jni/Android.mk
