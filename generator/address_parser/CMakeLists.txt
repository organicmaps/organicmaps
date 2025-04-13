project(address_parser)

set(SRC
  processor.cpp
  processor.hpp
  tiger_parser.cpp
  tiger_parser.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  generator
)


omim_add_executable(address_parser_tool parser_tool.cpp)
target_link_libraries(address_parser_tool
  ${PROJECT_NAME}
  gflags::gflags
)

omim_add_test_subdirectory(address_parser_tests)
