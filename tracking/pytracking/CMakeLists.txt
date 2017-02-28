project(pytracking)

check_pybindings()

set(
  SRC
  bindings.cpp
)

find_package(PythonLibs 2.7 REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS})

add_library(${PROJECT_NAME} MODULE ${SRC})

omim_link_libraries(${PROJECT_NAME} ${PYTHON_LIBRARIES} ${Boost_LIBRARIES} tracking coding geometry base)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
