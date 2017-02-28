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

DEFINES *= BOOST_SPIRIT_USE_PHOENIX_V3

HEADERS += opening_hours.hpp \
           opening_hours_parsers.hpp \
           parse_opening_hours.hpp \
           rules_evaluation_private.hpp \
           rules_evaluation.hpp

SOURCES += rules_evaluation.cpp \
           opening_hours_parsers_terminals.cpp \
           opening_hours.cpp \
           parse_opening_hours.cpp \
           parse_years.cpp \
           parse_weekdays.cpp \
           parse_weeks.cpp \
           parse_timespans.cpp \
           parse_months.cpp

