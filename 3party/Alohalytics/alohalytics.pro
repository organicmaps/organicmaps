TEMPLATE = app
CONFIG *= console c++11
CONFIG -= app_bundle qt

SOURCES += examples/cpp/example.cc \
           src/cpp/alohalytics.cc \

HEADERS += src/alohalytics.h \
           src/event_base.h \
           src/file_manager.h \
           src/gzip_wrapper.h \
           src/http_client.h \
           src/location.h \
           src/logger.h \
           src/messages_queue.h \

QMAKE_LFLAGS *= -lz

macx-* {
  HEADERS += src/alohalytics_objc.h
  OBJECTIVE_SOURCES += src/apple/http_client_apple.mm \
                       src/apple/alohalytics_objc.mm \

  QMAKE_OBJECTIVE_CFLAGS *= -fobjc-arc
  QMAKE_LFLAGS *=  -framework Foundation
}

macx-*|linux-* {
  SOURCES += src/posix/file_manager_posix_impl.cc
}

win* {
  SOURCES += src/windows/file_manager_windows_impl.cc
}

linux-* {
  SOURCES += src/posix/http_client_curl.cc
}

android-* {
  SOURCES += src/android/jni/jni_alohalytics.cc
}
