TEMPLATE = app
CONFIG *= console c++11
CONFIG -= app_bundle qt

SOURCES += examples/cpp/example.cc \
           src/alohalytics.cc \

HEADERS += src/alohalytics.h \
           src/http_client.h \
           src/gzip_wrapper.h \

QMAKE_LFLAGS *= -lz

macx-* {
  OBJECTIVE_SOURCES += src/http_client_apple.mm
  QMAKE_OBJECTIVE_CFLAGS *= -fobjc-arc
  QMAKE_LFLAGS *=  -framework Foundation
}
