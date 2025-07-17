project(platform_tests_support)

set(SRC
  async_gui_thread.hpp
  helpers.hpp
  scoped_dir.cpp
  scoped_dir.hpp
  scoped_file.cpp
  scoped_file.hpp
  scoped_mwm.cpp
  scoped_mwm.hpp
  test_socket.cpp
  test_socket.hpp
  writable_dir_changer.cpp
  writable_dir_changer.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  platform
  indexer  # feature::DataHeader::Save
)
