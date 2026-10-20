project(location_service)

if (NOT SKIP_QT_GUI AND PLATFORM_DESKTOP AND PLATFORM_LINUX)
  message("Building with Qt Positioning")
  find_package(Qt6 REQUIRED COMPONENTS Positioning)
  set(QT_LOCATION_SERVICE true)
elseif(NOT SKIP_QT_GUI AND PLATFORM_DESKTOP AND PLATFORM_MAC)
  set(APPLE_LOCATION_SERVICE true)
endif()

set(SRC
  location_service.cpp
  location_service.hpp
  position_filter.hpp
  $<$<BOOL:${APPLE_LOCATION_SERVICE}>:apple_location_service.mm>
  $<$<BOOL:${QT_LOCATION_SERVICE}>:qt_location_service.hpp>
  $<$<BOOL:${QT_LOCATION_SERVICE}>:qt_location_service.cpp>
)

omim_add_library(${PROJECT_NAME} ${SRC})

if (QT_LOCATION_SERVICE)
  target_compile_definitions(${PROJECT_NAME} PRIVATE "QT_LOCATION_SERVICE")
  target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Positioning)
  set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)
elseif(APPLE_LOCATION_SERVICE)
  target_compile_definitions(${PROJECT_NAME} PRIVATE "APPLE_LOCATION_SERVICE")
  target_link_libraries(${PROJECT_NAME} PRIVATE -framework\ CoreLocation)
endif()

if (BUILD_TESTING AND PLATFORM_DESKTOP)
  set(TEST_SRC position_filter_tests.cpp)
  set(TEST_OPTIONS NO_PLATFORM_INIT)
  if (TARGET Qt6::Positioning)
    list(APPEND TEST_OPTIONS REQUIRE_QT)
    list(APPEND TEST_SRC qt_location_service_tests.cpp)
    if (NOT QT_LOCATION_SERVICE)
      list(APPEND TEST_SRC qt_location_service.cpp qt_location_service.hpp)
    endif()
  endif()
  omim_add_test(location_service_tests ${TEST_SRC} ${TEST_OPTIONS})
  target_link_libraries(location_service_tests PRIVATE location_service)
  if (TARGET Qt6::Positioning)
    target_link_libraries(location_service_tests PRIVATE Qt6::Positioning)
    set_target_properties(location_service_tests PROPERTIES AUTOMOC ON)
  endif()
endif()
