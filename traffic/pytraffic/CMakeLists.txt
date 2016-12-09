project(pytraffic)

check_pybindings()

set(
  SRC
  bindings.cpp
)

# Suppress boost-python warnings
add_compile_options(
  "-Wno-unused-local-typedef"
)

set(Boost_USE_MULTITHREADED ON)

# For macOS we can use static linking, on Linux we can't.
if (PLATFORM_MAC)
  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_STATIC_RUNTIME ON)
endif()

find_package(PythonLibs 2.7 REQUIRED)
find_package(Boost 1.54 REQUIRED COMPONENTS python)
include_directories(${PYTHON_INCLUDE_DIRS})

add_library(${PROJECT_NAME} MODULE ${SRC})

if (PLATFORM_MAC)
  omim_link_libraries(
    ${PROJECT_NAME}
    ${Qt5Widgets_LIBRARIES}
    "-framework QuartzCore"
  )
endif()

if (PLATFORM_WIN OR PLATFORM_LINUX)
  omim_link_libraries(
    ${PROJECT_NAME}
    ${Qt5Widgets_LIBRARIES}
  )
endif()

omim_link_libraries(
  ${PROJECT_NAME}
  traffic
  routing
  indexer
  editor
  platform
  geometry
  coding
  base
  stats_client
  jansson
  oauthcpp
  protobuf
  pugixml
  opening_hours
  ${PYTHON_LIBRARIES}
  ${Boost_LIBRARIES}
  ${LIBZ}
)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
