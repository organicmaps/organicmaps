project(base_tests)

set(SRC
  assert_test.cpp
  beam_tests.cpp
  bidirectional_map_tests.cpp
  bits_test.cpp
  buffer_vector_test.cpp
  cache_test.cpp
  cancellable_tests.cpp
  checked_cast_tests.cpp
  clustering_map_tests.cpp
  collection_cast_test.cpp
  containers_test.cpp
  control_flow_tests.cpp
  exception_tests.cpp
  fifo_cache_test.cpp
  file_name_utils_tests.cpp
  geo_object_id_tests.cpp
  levenshtein_dfa_test.cpp
  linked_map_tests.cpp
  logging_test.cpp
  lru_cache_tests.cpp
  math_test.cpp
  matrix_test.cpp
  mem_trie_test.cpp
  message_test.cpp
  newtype_test.cpp
  non_intersecting_intervals_tests.cpp
  observer_list_test.cpp
  range_iterator_test.cpp
  ref_counted_tests.cpp
  regexp_test.cpp
  rolling_hash_test.cpp
  scope_guard_test.cpp
  small_set_test.cpp
  stl_helpers_tests.cpp
  string_utils_test.cpp
  suffix_array_tests.cpp
  sunrise_sunset_test.cpp
  thread_pool_computational_tests.cpp
  thread_pool_delayed_tests.cpp
  thread_pool_tests.cpp
  thread_safe_queue_tests.cpp
  threaded_list_test.cpp
  threads_test.cpp
  timer_test.cpp
  uni_string_dfa_test.cpp
  visitor_tests.cpp
)

# Windows does not have timegm function
if(NOT WIN32)
  list(APPEND SRC "timegm_test.cpp")
endif()

omim_add_test(${PROJECT_NAME} ${SRC} NO_PLATFORM_INIT)

target_link_libraries(${PROJECT_NAME} base)
