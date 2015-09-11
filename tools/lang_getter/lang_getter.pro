#-------------------------------------------------
#
# Project created by QtCreator 2011-11-18T08:50:14
#
#-------------------------------------------------

QT += core network xml

QT -= gui

TARGET = lang_getter
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    pagedownloader.cpp \
    logging.cpp \
    mainmanager.cpp \
    stringparser.cpp

HEADERS += \
    pagedownloader.h \
    logging.h \
    mainmanager.h \
    stringparser.h
