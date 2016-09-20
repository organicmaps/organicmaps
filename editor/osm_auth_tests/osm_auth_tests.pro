TARGET = osm_auth_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = editor platform_tests_support platform geometry coding base \
               stats_client pugixml oauthcpp tomcrypt

include($$ROOT_DIR/common.pri)

QT *= core

HEADERS += \

SOURCES += \
    $$ROOT_DIR/testing/testingmain.cpp \
    osm_auth_tests.cpp \
