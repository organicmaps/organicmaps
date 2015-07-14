# Coding tests.
TARGET = coding_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = coding base bzip2 zlib tomcrypt

include($$ROOT_DIR/common.pri)

win32-g++: LIBS *= -lpthread

SOURCES += ../../testing/testingmain.cpp \
    arithmetic_codec_test.cpp \
    base64_for_user_id_test.cpp \
    base64_test.cpp \
    bit_streams_test.cpp \
#    blob_storage_test.cpp \
    bzip2_test.cpp \
    coder_util_test.cpp \
    compressed_bit_vector_test.cpp \
#    compressed_varnum_vector_test.cpp \
    dd_vector_test.cpp \
    diff_test.cpp \
    endianness_test.cpp \
    file_container_test.cpp \
    file_data_test.cpp \
    file_sort_test.cpp \
    file_utils_test.cpp \
    gzip_test.cpp \
    hex_test.cpp \
    mem_file_reader_test.cpp \
    mem_file_writer_test.cpp \
    multilang_utf8_string_test.cpp \
    png_decoder_test.cpp \
    reader_cache_test.cpp \
    reader_test.cpp \
    reader_writer_ops_test.cpp \
    sha2_test.cpp \
    timsort_test.cpp \
    trie_test.cpp \
    uri_test.cpp \
    url_encode_test.cpp \
    value_opt_string_test.cpp \
    var_record_reader_test.cpp \
    var_serial_vector_test.cpp \
    varint_test.cpp \
#    varint_vector_test.cpp \
    writer_test.cpp \
    zip_creator_test.cpp \
    zip_reader_test.cpp \

HEADERS += \
    coder_test.hpp \
    compressor_test_utils.hpp \
    reader_test.hpp \
