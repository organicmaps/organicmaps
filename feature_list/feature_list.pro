# Feature List Tool

ROOT_DIR = ..
DEPENDENCIES = map search_tests_support search search_quality storage indexer platform editor geometry \
               coding base tomcrypt jansson protobuf stats_client succinct opening_hours pugixml

include($$ROOT_DIR/common.pri)

CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

# needed for Platform::WorkingDir() and unicode combining
QT *= core network

macx-* {
  LIBS *= "-framework IOKit" "-framework SystemConfiguration"
}

SOURCES += feature_list.cpp
