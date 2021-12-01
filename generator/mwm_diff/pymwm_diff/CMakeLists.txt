project(pymwm_diff)

set(SRC bindings.cpp)

omim_add_library(${PROJECT_NAME} MODULE ${SRC})

target_link_libraries(${PROJECT_NAME} mwm_diff)