project(storage)

set(SRC
  country.hpp
  country_decl.cpp
  country_decl.hpp
  country_info_getter.cpp
  country_info_getter.hpp
  country_name_getter.cpp
  country_name_getter.hpp
  country_info_reader_light.cpp
  country_info_reader_light.hpp
  country_parent_getter.cpp
  country_parent_getter.hpp
  country_tree.cpp
  country_tree.hpp
  country_tree_helpers.cpp
  country_tree_helpers.hpp
  diff_scheme/apply_diff.cpp
  diff_scheme/apply_diff.hpp
  diff_scheme/diffs_data_source.cpp
  diff_scheme/diffs_data_source.hpp
  diff_scheme/diff_scheme_loader.cpp
  diff_scheme/diff_scheme_loader.hpp
  diff_scheme/diff_types.hpp
  downloader.hpp
  downloader_search_params.hpp
  downloader_queue_interface.hpp
  downloader_queue_universal.cpp
  downloader_queue_universal.hpp
  downloading_policy.cpp
  downloading_policy.hpp
  map_files_downloader.cpp
  map_files_downloader.hpp
  map_files_downloader_with_ping.cpp
  map_files_downloader_with_ping.hpp
  queued_country.cpp
  queued_country.hpp
  pinger.cpp
  pinger.hpp
  routing_helpers.cpp
  routing_helpers.hpp
  storage.cpp
  storage.hpp
  storage_defines.cpp
  storage_defines.hpp
  storage_helpers.cpp
  storage_helpers.hpp
)

if (PLATFORM_IPHONE)
  append(SRC
    background_downloading/downloader_adapter_ios.h
    background_downloading/downloader_adapter_ios.mm
    background_downloading/downloader_queue.hpp
  )
else()
  append(SRC
    http_map_files_downloader.cpp
    http_map_files_downloader.hpp
  )
endif()

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  platform
  coding
  mwm_diff
  cppjansson
)

omim_add_test_subdirectory(storage_tests)
omim_add_test_subdirectory(storage_integration_tests)
