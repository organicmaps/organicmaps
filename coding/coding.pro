# Coding project.
TARGET = coding
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/tomcrypt/src/headers

SOURCES += \
    $$ROOT_DIR/3party/lodepng/lodepng.cpp \
    arithmetic_codec.cpp \
    base64.cpp \
#    blob_indexer.cpp \
#    blob_storage.cpp \
    compressed_bit_vector.cpp \
#    compressed_varnum_vector.cpp \
    file_container.cpp \
    file_name_utils.cpp \
    file_reader.cpp \
    file_writer.cpp \
    hex.cpp \
    huffman.cpp \
    internal/file_data.cpp \
    mmap_reader.cpp \
    multilang_utf8_string.cpp \
    png_memory_encoder.cpp \
    reader.cpp \
    reader_streambuf.cpp \
    reader_writer_ops.cpp \
    sha2.cpp \
    uri.cpp \
#    varint_vector.cpp \
    zip_creator.cpp \
    zip_reader.cpp \

HEADERS += \
    $$ROOT_DIR/3party/expat/expat_impl.h \
    $$ROOT_DIR/3party/lodepng/lodepng.hpp \
    $$ROOT_DIR/3party/lodepng/lodepng_io.hpp \
    $$ROOT_DIR/3party/lodepng/lodepng_io_private.hpp \
    arithmetic_codec.hpp \
    base64.hpp \
    bit_streams.hpp \
#    blob_indexer.hpp \
#    blob_storage.hpp \
    buffer_reader.hpp \
    byte_stream.hpp \
    coder.hpp \
    coder_util.hpp \
    compressed_bit_vector.hpp \
#    compressed_varnum_vector.hpp \
    constants.hpp \
    dd_vector.hpp \
    diff.hpp \
    diff_patch_common.hpp \
    endianness.hpp \
    file_container.hpp \
    file_name_utils.hpp \
    file_reader.hpp \
    file_reader_stream.hpp \
    file_sort.hpp \
    file_writer.hpp \
    file_writer_stream.hpp \
    hex.hpp \
    huffman.hpp \
    internal/file64_api.hpp \
    internal/file_data.hpp \
    matrix_traversal.hpp \
    mmap_reader.hpp \
    multilang_utf8_string.hpp \
    parse_xml.hpp \
    png_memory_encoder.hpp \
    polymorph_reader.hpp \
    read_write_utils.hpp \
    reader.hpp \
    reader_cache.hpp \
    reader_streambuf.hpp \
    reader_wrapper.hpp \
    reader_writer_ops.hpp \
    sha2.hpp \
    streams.hpp \
    streams_common.hpp \
    streams_sink.hpp \
    succinct_trie.hpp \
    succinct_trie_builder.hpp \
    succinct_trie_reader.hpp \
    trie.hpp \
    trie_builder.hpp \
    trie_reader.hpp \
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
    internal/xmlparser.hpp \
