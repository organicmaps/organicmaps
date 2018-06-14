project(pylocal_ads)

set(
  SRC
  bindings.cpp
)

omim_add_library(${PROJECT_NAME} MODULE ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  ${PYTHON_LIBRARIES}
  ${Boost_LIBRARIES}
  local_ads
  base
)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
