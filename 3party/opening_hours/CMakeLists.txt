project(opening_hours)

set(SRC
  opening_hours.hpp
  opening_hours.cpp
  oh/parser.hpp
  oh/eval.hpp
  oh/convert.hpp
  oh/convert.cpp
)

omim_add_library(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE om::timezone)

target_include_directories(${PROJECT_NAME} INTERFACE .)

# osmoh facade + issue-regression tests (OM test framework).
omim_add_test_subdirectory(opening_hours_tests)

# Port golden-vector tests (transcribed from the opening-hours-rs Rust suite).
# Self-contained: own main + assertion framework, no OM deps.
foreach(oh_test oh/parser_test.cpp oh/eval_test.cpp)
  get_filename_component(oh_test_name ${oh_test} NAME_WE)
  add_executable(${oh_test_name} ${oh_test})
  target_include_directories(${oh_test_name} PRIVATE ${OMIM_ROOT})
  add_test(NAME ${oh_test_name} COMMAND ${oh_test_name})
  set_tests_properties(${oh_test_name} PROPERTIES LABELS "omim-test")
endforeach()
