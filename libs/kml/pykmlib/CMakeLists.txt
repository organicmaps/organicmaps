project(pykmlib)

set(
  SRC
  bindings.cpp
)

include_directories(${CMAKE_BINARY_DIR})

omim_add_library(${PROJECT_NAME} MODULE ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  ${Boost_LIBRARIES}
  kml
  indexer
  editor
  platform
  coding
  geometry
  base
  ICU::i18n
  cppjansson
  protobuf
  pugixml
  expat::expat
  succinct
)

link_qt5_core(${PROJECT_NAME})

if (PLATFORM_MAC)
  omim_link_libraries(${PROJECT_NAME} "-Wl,-undefined,dynamic_lookup")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
