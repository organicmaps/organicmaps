TEMPLATE = app
CONFIG *= console c++11
CONFIG -= app_bundle qt

SOURCES += examples/cpp/example.cc \
           src/cpp/alohalytics.cc \

HEADERS += src/alohalytics.h \
           src/event_base.h \
           src/http_client.h \
           src/gzip_wrapper.h \
           src/logger.h \
           src/location.h \

QMAKE_LFLAGS *= -lz

macx-* {
  HEADERS += src/alohalytics_objc.h
  OBJECTIVE_SOURCES += src/apple/http_client_apple.mm \
                       src/apple/alohalytics_objc.mm \

  QMAKE_OBJECTIVE_CFLAGS *= -fobjc-arc
  QMAKE_LFLAGS *=  -framework Foundation
}
