project(coding_tests)

set(SRC
  base64_test.cpp
  bit_streams_test.cpp
  bwt_coder_tests.cpp
  bwt_tests.cpp
  compressed_bit_vector_test.cpp
  csv_reader_test.cpp
  dd_vector_test.cpp
  diff_test.cpp
  elias_coder_test.cpp
  endianness_test.cpp
  file_data_test.cpp
  file_sort_test.cpp
  files_container_tests.cpp
  fixed_bits_ddvector_test.cpp
  geometry_coding_test.cpp
  hex_test.cpp
  huffman_test.cpp
  map_uint32_to_val_tests.cpp
  mem_file_reader_test.cpp
  mem_file_writer_test.cpp
  move_to_front_tests.cpp
  png_decoder_test.cpp
  point_coding_tests.cpp
  reader_cache_test.cpp
  reader_test.cpp
  reader_test.hpp
  reader_writer_ops_test.cpp
  serdes_json_test.cpp
  simple_dense_coding_test.cpp
  sha1_test.cpp
  sparse_vector_tests.cpp
  string_utf8_multilang_tests.cpp
  succinct_ef_test.cpp
  succinct_mapper_test.cpp
  test_polylines.cpp
  test_polylines.hpp
  text_storage_tests.cpp
  traffic_test.cpp
  url_tests.cpp
  value_opt_string_test.cpp
  var_record_reader_test.cpp
  var_serial_vector_test.cpp
  varint_test.cpp
  writer_test.cpp
  xml_parser_tests.cpp
  zip_creator_test.cpp
  zip_reader_test.cpp
  zlib_test.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  platform_tests_support  # For csv_reader_test (TODO: Move ScopedFile into a base header)
  geometry                # For traffic_test and serdes_json_test (TODO: Remove dependency)
  coding
)
