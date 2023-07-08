project(pytracking)

set(
  SRC
  bindings.cpp
)

include_directories(${CMAKE_BINARY_DIR})

omim_add_library(${PROJECT_NAME} MODULE ${SRC})

omim_link_libraries(
    ${PROJECT_NAME} 
    ${Boost_LIBRARIES}
    tracking
    coding
    geometry
    base
)

if (PLATFORM_MAC)
    omim_link_libraries(${PROJECT_NAME} "-Wl,-undefined,dynamic_lookup")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
