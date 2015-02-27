#-------------------------------------------------
#
# Project created by QtCreator 2015-02-26T15:21:15
#
#-------------------------------------------------

TARGET = succinct

ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

TEMPLATE = lib
CONFIG += staticlib

SOURCES += \
    bp_vector.cpp \
    rs_bit_vector.cpp

HEADERS += \
    bit_vector.hpp \
    bp_vector.hpp \
    broadword.hpp \
    cartesian_tree.hpp \
    darray.hpp \
    darray64.hpp \
    elias_fano_compressed_list.hpp \
    elias_fano_list.hpp \
    elias_fano.hpp \
    forward_enumerator.hpp \
    gamma_bit_vector.hpp \
    gamma_vector.hpp \
    intrinsics.hpp \
    mappable_vector.hpp \
    mapper.hpp \
    nibble_vector.hpp \
    rs_bit_vector.hpp \
    tables.hpp \
    topk_vector.hpp \
    util.hpp \
    vbyte.hpp
