TARGET = local_ads_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = local_ads platform_tests_support platform coding base stats_client

include($$ROOT_DIR/common.pri)

macx-* {
  LIBS *= "-framework IOKit" "-framework SystemConfiguration"
}

QT *= core

HEADERS += \

SOURCES += \
    $$ROOT_DIR/testing/testingmain.cpp \
    campaign_serialization_test.cpp \
    local_ads_helpers_tests.cpp \
