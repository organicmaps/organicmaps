# Coding project.
TARGET = coding
TEMPLATE = lib
CONFIG += staticlib warn_on
INCLUDEPATH += ../3party/icu/common ../3party/icu/i18n

DEFINES *= U_DISABLE_RENAMING

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

SOURCES += \
    base64.cpp \
    compressed_bit_vector.cpp \
    csv_reader.cpp \
    file_container.cpp \
    file_name_utils.cpp \
    file_reader.cpp \
    file_writer.cpp \
    hex.cpp \
    huffman.cpp \
    internal/file_data.cpp \
    mmap_reader.cpp \
    multilang_utf8_string.cpp \
    point_to_integer.cpp \
    reader.cpp \
    reader_streambuf.cpp \
    reader_writer_ops.cpp \
    simple_dense_coding.cpp \
    traffic.cpp \
    transliteration.cpp \
    uri.cpp \
#    varint_vector.cpp \
    zip_creator.cpp \
    zip_reader.cpp \
    zlib.cpp \

HEADERS += \
    $$ROOT_DIR/3party/expat/expat_impl.h \
    base64.hpp \
    bit_streams.hpp \
    buffer_reader.hpp \
    bwt_coder.hpp \
    byte_stream.hpp \
    coder.hpp \
    coder_util.hpp \
    compressed_bit_vector.hpp \
    constants.hpp \
    csv_reader.hpp \
    dd_vector.hpp \
    diff.hpp \
    diff_patch_common.hpp \
    elias_coder.hpp \
    endianness.hpp \
    file_container.hpp \
    file_name_utils.hpp \
    file_reader.hpp \
    file_reader_stream.hpp \
    file_sort.hpp \
    file_writer.hpp \
    file_writer_stream.hpp \
    fixed_bits_ddvector.hpp \
    hex.hpp \
    huffman.hpp \
    internal/file64_api.hpp \
    internal/file_data.hpp \
    internal/xmlparser.hpp \
    matrix_traversal.hpp \
    memory_region.hpp \
    mmap_reader.hpp \
    multilang_utf8_string.hpp \
    parse_xml.hpp \
    point_to_integer.hpp \
    polymorph_reader.hpp \
    read_write_utils.hpp \
    reader.hpp \
    reader_cache.hpp \
    reader_streambuf.hpp \
    reader_wrapper.hpp \
    reader_writer_ops.hpp \
    simple_dense_coding.hpp \
    streams.hpp \
    streams_common.hpp \
    streams_sink.hpp \
    succinct_mapper.hpp \
    text_storage.hpp \
    traffic.hpp \
    transliteration.hpp \
    uri.hpp \
    url_encode.hpp \
    value_opt_string.hpp \
    var_record_reader.hpp \
    var_serial_vector.hpp \
    varint.hpp \
    varint_misc.hpp \
#    varint_vector.hpp \
    write_to_sink.hpp \
    writer.hpp \
    zip_creator.hpp \
    zip_reader.hpp \
    zlib.hpp
