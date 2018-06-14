project(pymwm_diff)

set(
  SRC
  bindings.cpp
)

omim_add_library(${PROJECT_NAME} MODULE ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  mwm_diff
  bsdiff
  platform
  coding
  base
  stats_client
  ${PYTHON_LIBRARIES}
  ${Boost_LIBRARIES}
  ${LIBZ}
)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
