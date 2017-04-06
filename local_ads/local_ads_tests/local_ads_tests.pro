TARGET = local_ads_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = local_ads platform_tests_support stats_client coding platform base

include($$ROOT_DIR/common.pri)

macx-* {
  LIBS *= "-framework IOKit" "-framework SystemConfiguration"
}

HEADERS += \

SOURCES += \
    $$ROOT_DIR/testing/testingmain.cpp \
    campaign_serialization_test.cpp \
    local_ads_helpers_tests.cpp \
