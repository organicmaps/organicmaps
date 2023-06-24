project(booking_quality_check)

set(SRC booking_quality_check.cpp)
#set(SRC booking_addr_match.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  generator
  gflags::gflags
)
