TARGET = routing_benchmarks
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../../
DEPENDENCIES = map routing traffic routing_common search storage indexer platform editor geometry coding base \
               osrm jansson protobuf stats_client succinct pugixml

macx-*: LIBS *= "-framework IOKit"

include($$ROOT_DIR/common.pri)

QT *= core

macx-* {
  QT *= gui widgets # needed for QApplication with event loop, to test async events (downloader, etc.)
  LIBS *= "-framework IOKit" "-framework QuartzCore" "-framework Cocoa" "-framework SystemConfiguration"
}
win32*|linux* {
  QT *= network
}

SOURCES += \
  ../../testing/testingmain.cpp \
  bicycle_routing_tests.cpp \
  helpers.cpp \
  pedestrian_routing_tests.cpp \

HEADERS += \
  helpers.hpp \
