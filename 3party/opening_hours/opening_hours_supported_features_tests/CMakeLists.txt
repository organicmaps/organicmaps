project(opening_hours_supported_features_tests)

set(SRC opening_hours_supported_features_tests.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} opening_hours)
# Silence boost::test warnings.
target_compile_options(${PROJECT_NAME} PRIVATE
  $<$<CXX_COMPILER_ID:AppleClang>:-Wno-deprecated-declarations>
  $<$<CXX_COMPILER_ID:AppleClang>:-Wno-unused-but-set-variable>
)
