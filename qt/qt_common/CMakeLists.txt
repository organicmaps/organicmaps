project(qt_common)

QT5_ADD_RESOURCES(RESOURCES res/resources_common.qrc)

set_property(SOURCE qrc_resources_common.cpp PROPERTY SKIP_AUTOGEN ON)

include_directories(
  ${OMIM_ROOT}/3party/jansson/src
)

set(
  SRC
  helpers.cpp
  helpers.hpp
  map_widget.cpp
  map_widget.hpp
  proxy_style.cpp
  proxy_style.hpp
  qtoglcontext.cpp
  qtoglcontext.hpp
  qtoglcontextfactory.cpp
  qtoglcontextfactory.hpp
  scale_slider.cpp
  scale_slider.hpp
  spinner.cpp
  spinner.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC} ${RESOURCES})
