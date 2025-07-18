project(feature_list_generator)

set(SRC feature_list.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  mwm_diff
  generator
  search
  platform
  indexer
)
