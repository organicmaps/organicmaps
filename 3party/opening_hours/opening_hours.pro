#-------------------------------------------------
#
# Project created by QtCreator 2015-03-27T13:21:18
#
#-------------------------------------------------


TARGET = opening_hours
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

HEADERS += rules_evaluation.hpp \
           rules_evaluation_private.hpp \
           opening_hours_parsers.hpp \
           opening_hours_parsers_terminals.hpp \
           opening_hours.hpp \
           parse_opening_hours.hpp

SOURCES += rules_evaluation.cpp \
           opening_hours.cpp \
           parse_opening_hours.cpp
