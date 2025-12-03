project(mwm_viewer)

set(SRC mwm_viewer.cpp)

omim_add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    search
    platform
    indexer
)
