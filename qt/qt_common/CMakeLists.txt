project(qt_common)

QT6_ADD_RESOURCES(RESOURCES res/resources_common.qrc)

set_property(SOURCE qrc_resources_common.cpp PROPERTY SKIP_AUTOGEN ON)

set(SRC
  helpers.cpp
  helpers.hpp
  map_widget.cpp
  map_widget.hpp
  map_widget_container.cpp
  map_widget_container.hpp
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
  text_dialog.cpp
  text_dialog.hpp
  translations.cpp
  translations.hpp
)

omim_add_library(${PROJECT_NAME} ${SRC} ${RESOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    map
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
)

if (BUILD_TESTING)
  omim_add_test(map_widget_container_tests NO_PLATFORM_INIT map_widget_container_tests.cpp)
  target_link_libraries(map_widget_container_tests PRIVATE qt_common)
  set_tests_properties(map_widget_container_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

  omim_add_test(qt_drape_tests qtoglcontext_tests.cpp NO_PLATFORM_INIT)
  omim_add_test(qt_map_drape_tests map_widget_tests.cpp)
  foreach(test_target IN ITEMS qt_drape_tests qt_map_drape_tests)
    target_link_libraries(${test_target} PRIVATE qt_common qt_tstfrm)
    if (PLATFORM_MAC)
      # Native Cocoa supports offscreen GL surfaces; the offscreen QPA plugin cannot create a GL context.
      set_tests_properties(${test_target} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=cocoa"
        FAIL_REGULAR_EXPRESSION "OpenGL context is not available"
      )
    else()
      set_tests_properties(${test_target} PROPERTIES SKIP_REGULAR_EXPRESSION "OpenGL context is not available")
    endif()
  endforeach()
endif()
