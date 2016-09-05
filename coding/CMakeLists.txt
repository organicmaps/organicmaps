include_directories(${PROJECT_SOURCE_DIR}/3party/tomcrypt/src/headers)

set(
    HEADERS
    base64.hpp
    bit_streams.hpp
    buffer_reader.hpp
    byte_stream.hpp
    coder.hpp
    coder_util.hpp
    compressed_bit_vector.hpp
    constants.hpp
    dd_vector.hpp
    diff.hpp
    diff_patch_common.hpp
    elias_coder.hpp
    endianness.hpp
    file_container.hpp
    file_name_utils.hpp
    file_reader.hpp
    file_sort.hpp
    file_writer.hpp
    fixed_bits_ddvector.hpp
    hex.hpp
    huffman.hpp
    matrix_traversal.hpp
    memory_region.hpp
    mmap_reader.hpp
    multilang_utf8_string.hpp
    parse_xml.hpp
    png_memory_encoder.hpp
    polymorph_reader.hpp
    read_write_utils.hpp
    reader.hpp
    reader_cache.hpp
    reader_streambuf.hpp
    reader_wrapper.hpp
    reader_writer_ops.hpp
    sha2.hpp
    simple_dense_coding.hpp
    streams.hpp
    streams_common.hpp
    streams_sink.hpp
    succinct_mapper.hpp
    uri.hpp
    url_encode.hpp
    value_opt_string.hpp
    var_record_reader.hpp
    var_serial_vector.hpp
    varint.hpp
    varint_misc.hpp
#    varint_vector.hpp
    write_to_sink.hpp
    writer.hpp
    zip_creator.hpp
    zip_reader.hpp
)

set(
    SOURCES
    base64.cpp
    compressed_bit_vector.cpp
    file_container.cpp
    file_name_utils.cpp
    file_reader.cpp
    file_writer.cpp
    hex.cpp
    huffman.cpp
    mmap_reader.cpp
    multilang_utf8_string.cpp
    png_memory_encoder.cpp
    reader.cpp
    reader_streambuf.cpp
    reader_writer_ops.cpp
    sha2.cpp
    simple_dense_coding.cpp
    uri.cpp
#    varint_vector.cpp
    zip_creator.cpp
    zip_reader.cpp
)

set(SRC ${HEADERS} ${SOURCES})

omim_add_library(coding "${SRC}")

add_subdirectory(coding_tests)
