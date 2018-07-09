project(osrm)

add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
include_directories(
  ${OMIM_ROOT}/3party/osrm/osrm-backend/include
  ${OMIM_ROOT}/3party/osrm/osrm-backend/third_party
)

add_clang_compile_options(
  "-Wno-unused-local-typedef"
  "-Wno-overloaded-virtual"
)

set(
  SRC
  boost_stub.cpp
  osrm-backend/data_structures/coordinate.cpp
  osrm-backend/data_structures/coordinate_calculation.hpp
  osrm-backend/data_structures/coordinate_calculation.cpp
  osrm-backend/data_structures/search_engine_data.cpp
  osrm-backend/data_structures/phantom_node.cpp
  osrm-backend/util/mercator.cpp
  osrm-backend/util/mercator.hpp
)

# if (APPLE)
#  target_compile_options(${PROJECT_NAME} PUBLIC "-fobjc-arc")
#endif()

add_library(${PROJECT_NAME} ${SRC})

#omim_add_test_subdirectory(osrm_tests)
#omim_add_test(${project} ${SRC})
# omim_link_libraries(osrm ...)
