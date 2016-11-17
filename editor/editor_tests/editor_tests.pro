TARGET = editor_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = editor platform_tests_support platform geometry coding base \
               stats_client opening_hours pugixml oauthcpp

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
    config_loader_test.cpp \
    editor_config_test.cpp \
    editor_notes_test.cpp \
    opening_hours_ui_test.cpp \
    osm_feature_matcher_test.cpp \
    ui2oh_test.cpp \
    user_stats_test.cpp \
    xml_feature_test.cpp \
