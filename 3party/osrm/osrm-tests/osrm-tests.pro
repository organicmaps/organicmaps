TARGET = osrm_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../../..
DEPENDENCIES = osrm base

include($$ROOT_DIR/common.pri)

INCLUDEPATH = ../../boost \
              ../osrm-backend \
              ../osrm-backend/Include \
              ../../tbb/include \

LIBS *= -L$$_PRO_FILE_PWD_/libs.tmp.darwin.x86_64 -lboost_filesystem -lboost_thread -lboost_system

HEADERS += \

SOURCES += \
    $$ROOT_DIR/testing/testingmain.cpp \
    osrm_smoke_test.cpp \
