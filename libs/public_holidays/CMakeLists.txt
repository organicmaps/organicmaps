project(public_holidays)

set(SRC
  holidays.cpp
  holidays.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC})
add_library(om::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  base
)
