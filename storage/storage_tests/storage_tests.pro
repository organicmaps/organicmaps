# Storage library tests.

TARGET = storage_tests
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = storage indexer geometry platform coding base

include($$ROOT_DIR/common.pri)

QT += core

HEADERS +=

SOURCES += \
  ../../testing/testingmain.cpp \
  country_test.cpp \
  simple_tree_test.cpp \
