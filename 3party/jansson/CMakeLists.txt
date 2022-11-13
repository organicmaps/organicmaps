set(JANSSON_EXAMPLES OFF)
set(JANSSON_BUILD_DOCS OFF)
set(JANSSON_WITHOUT_TESTS OFF)
set(JANSSON_INSTALL OFF)

add_subdirectory(jansson)

target_sources(jansson
  PRIVATE
    jansson_handle.cpp
    myjansson.cpp
  PUBLIC
    jansson_handle.hpp
    myjansson.hpp
)

target_include_directories(jansson PUBLIC jansson/src .)
