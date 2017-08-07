# Generator binary

ROOT_DIR = ../..

DEPENDENCIES = generator routing traffic routing_common search storage indexer editor platform geometry \
               coding base freetype expat jansson protobuf osrm stats_client \
               minizip succinct pugixml tess2 gflags oauthcpp icu
include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/gflags/src

CONFIG += console warn_on
!CONFIG(map_designer_standalone) {
  CONFIG -= app_bundle
}
TEMPLATE = app

# needed for Platform::WorkingDir() and unicode combining
QT *= core

!iphone*:!android*:!tizen:!macx-* {
  QT *= network
}

macx-* {
  LIBS *= "-framework IOKit" "-framework SystemConfiguration"
}

SOURCES += \
    generator_tool.cpp \

HEADERS += \
