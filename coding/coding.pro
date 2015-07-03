# Coding project.
TARGET = coding
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/tomcrypt/src/headers $$ROOT_DIR/3party/zlib $$ROOT_DIR/3party/expat/lib

SOURCES += \
    internal/file_data.cpp \
    hex.cpp \
    file_reader.cpp \
    file_writer.cpp \
    lodepng.cpp \
    file_container.cpp \
    bzip2_compressor.cpp \
    gzip_compressor.cpp \
    timsort/timsort.cpp \
    base64.cpp \
    sha2.cpp \
    multilang_utf8_string.cpp \
    reader.cpp \
    zip_reader.cpp \
    mmap_reader.cpp \
    reader_streambuf.cpp \
    reader_writer_ops.cpp \
#    blob_indexer.cpp \
#    blob_storage.cpp \
    uri.cpp \
    zip_creator.cpp \
    file_name_utils.cpp \
#    varint_vector.cpp \
    arithmetic_codec.cpp \
    compressed_bit_vector.cpp \
#    compressed_varnum_vector.cpp \
    png_memory_encoder.cpp \
    huffman.cpp \

HEADERS += \
    internal/xmlparser.hpp \
    internal/expat_impl.h \
    internal/file_data.hpp \
    internal/file64_api.hpp \
    parse_xml.hpp \
    varint.hpp \
    endianness.hpp \
    byte_stream.hpp \
    var_serial_vector.hpp \
    hex.hpp \
    dd_vector.hpp \
    writer.hpp \
    write_to_sink.hpp \
    reader.hpp \
    diff.hpp \
    diff_patch_common.hpp \
    lodepng.hpp \
    lodepng_io.hpp \
    lodepng_io_private.hpp \
    var_record_reader.hpp \
    file_sort.hpp \
    file_reader.hpp \
    file_writer.hpp \
    reader_cache.hpp \
    buffer_reader.hpp \
    streams.hpp \
    streams_sink.hpp \
    streams_common.hpp \
    file_container.hpp \
    polymorph_reader.hpp \
    coder.hpp \
    coder_util.hpp \
    bzip2_compressor.hpp \
    gzip_compressor.hpp \
    timsort/timsort.hpp \
    base64.hpp \
    sha2.hpp \
    value_opt_string.hpp \
    multilang_utf8_string.hpp \
    url_encode.hpp \
    zip_reader.hpp \
    trie.hpp \
    trie_builder.hpp \
    trie_reader.hpp \
    mmap_reader.hpp \
    read_write_utils.hpp \
    file_reader_stream.hpp \
    file_writer_stream.hpp \
    reader_streambuf.hpp \
    reader_writer_ops.hpp \
    reader_wrapper.hpp \
#    blob_indexer.hpp \
#    blob_storage.hpp \
    uri.hpp \
    zip_creator.hpp \
    file_name_utils.hpp \
    constants.hpp \
    matrix_traversal.hpp \
#    varint_vector.hpp \
    arithmetic_codec.hpp \
    compressed_bit_vector.hpp \
#    compressed_varnum_vector.hpp \
    varint_misc.hpp \
    bit_streams.hpp \
    png_memory_encoder.hpp \
    huffman.hpp \
