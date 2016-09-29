set(OPENING_HOURS_INCLUDE ../opening_hours)

include_directories(${PROJECT_SOURCE_DIR})


set(
  SRC
  opening_hours_tests.cpp
)

add_executable(opening_hours_tests ${SRC})
omim_link_libraries(opening_hours_tests base opening_hours)
