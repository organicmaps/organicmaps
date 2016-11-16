TARGET = osm_auth_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = editor platform_tests_support platform geometry coding base \
               stats_client pugixml oauthcpp

include($$ROOT_DIR/common.pri)

QT *= core

macx-* {
  QT *= gui widgets # needed for QApplication with event loop, to test async events (downloader, etc.)
  LIBS *= "-framework IOKit" "-framework QuartzCore" "-framework Cocoa" "-framework SystemConfiguration"
}
win32*|linux* {
  QT *= network
}

HEADERS += \

SOURCES += \
    $$ROOT_DIR/testing/testingmain.cpp \
    osm_auth_tests.cpp \
    server_api_test.cpp \
