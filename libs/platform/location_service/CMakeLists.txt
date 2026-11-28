project(location_service)

if (NOT SKIP_QT_GUI AND PLATFORM_DESKTOP AND PLATFORM_LINUX)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GEOCLUE REQUIRED IMPORTED_TARGET libgeoclue-2.0)
  set(GEOCLUE_LOCATION_SERVICE true)
elseif(NOT SKIP_QT_GUI AND PLATFORM_SAILFISH)
  # Sailfish OS has only GeoClue 1, which Qt Positioning supports.
  message("Building with Qt Positioning")
  find_package(Qt${OMIM_QT_VERSION} REQUIRED COMPONENTS Positioning)
  set(QT_LOCATION_SERVICE true)
elseif(NOT SKIP_QT_GUI AND PLATFORM_DESKTOP AND PLATFORM_MAC)
  set(APPLE_LOCATION_SERVICE true)
endif()

set(SRC
  location_service.cpp
  location_service.hpp
  position_filter.hpp
  $<$<BOOL:${APPLE_LOCATION_SERVICE}>:apple_location_service.mm>
  $<$<BOOL:${GEOCLUE_LOCATION_SERVICE}>:geoclue_location_service.cpp>
  $<$<BOOL:${GEOCLUE_LOCATION_SERVICE}>:geoclue_location_service.hpp>
  $<$<BOOL:${QT_LOCATION_SERVICE}>:qt_location_service.hpp>
  $<$<BOOL:${QT_LOCATION_SERVICE}>:qt_location_service.cpp>
)

omim_add_library(${PROJECT_NAME} ${SRC})

if (GEOCLUE_LOCATION_SERVICE)
  target_compile_definitions(${PROJECT_NAME} PRIVATE "GEOCLUE_LOCATION_SERVICE")
  # Older GClueSimple keeps the portal session open when it is stopped before the first fix.
  if (GEOCLUE_VERSION VERSION_GREATER_EQUAL 2.7.2)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "GEOCLUE_PORTAL")
  else()
    message(WARNING "libgeoclue ${GEOCLUE_VERSION} is older than 2.7.2: location will not use xdg-desktop-portal")
  endif()
  target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::GEOCLUE)
elseif(QT_LOCATION_SERVICE)
  target_compile_definitions(${PROJECT_NAME} PRIVATE "QT_LOCATION_SERVICE")
  target_link_libraries(${PROJECT_NAME} PRIVATE Qt${OMIM_QT_VERSION}::Positioning)
  set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)
elseif(APPLE_LOCATION_SERVICE)
  target_compile_definitions(${PROJECT_NAME} PRIVATE "APPLE_LOCATION_SERVICE")
  target_link_libraries(${PROJECT_NAME} PRIVATE -framework\ CoreLocation)
endif()

if (PLATFORM_SAILFISH AND NOT SKIP_QT_GUI)
  # Run by the Sailfish OS CI job, like qt_translations_check.
  omim_add_executable(location_service_check location_service_check.cpp)
  set_target_properties(location_service_check PROPERTIES EXCLUDE_FROM_ALL ON)
  target_link_libraries(location_service_check PRIVATE location_service base Qt${OMIM_QT_VERSION}::Positioning)
endif()

if (BUILD_TESTING AND PLATFORM_DESKTOP)
  set(TEST_SRC position_filter_tests.cpp)
  set(TEST_OPTIONS NO_PLATFORM_INIT)
  # The Qt adapter is used on Sailfish OS, which builds no tests. Keep it out of the GeoClue test binary,
  # which runs the GLib main context itself.
  if (TARGET Qt6::Positioning AND NOT GEOCLUE_LOCATION_SERVICE)
    set(QT_LOCATION_SERVICE_TESTS true)
    list(APPEND TEST_OPTIONS REQUIRE_QT)
    list(APPEND TEST_SRC qt_location_service.cpp qt_location_service.hpp qt_location_service_tests.cpp)
  endif()
  if (GEOCLUE_LOCATION_SERVICE)
    list(APPEND TEST_SRC fake_geoclue_services.hpp geoclue_location_service_tests.cpp)
  endif()
  omim_add_test(location_service_tests ${TEST_SRC} ${TEST_OPTIONS})
  target_link_libraries(location_service_tests PRIVATE location_service)
  if (QT_LOCATION_SERVICE_TESTS)
    target_link_libraries(location_service_tests PRIVATE Qt6::Positioning)
    set_target_properties(location_service_tests PROPERTIES AUTOMOC ON)
  endif()
  if (GEOCLUE_LOCATION_SERVICE)
    target_link_libraries(location_service_tests PRIVATE PkgConfig::GEOCLUE)
    set_tests_properties(location_service_tests PROPERTIES ENVIRONMENT "GTK_USE_PORTAL=0;G_DEBUG=fatal-criticals")
    if (GEOCLUE_VERSION VERSION_GREATER_EQUAL 2.7.2)
      # GClueSimple caches its backend selection: test the portal in a separate process.
      add_test(NAME location_service_portal_tests
        COMMAND location_service_tests --filter=GeoClueLocationService_AsyncLifecycle)
      set_tests_properties(location_service_portal_tests PROPERTIES
        LABELS omim-test ENVIRONMENT "GTK_USE_PORTAL=1;G_DEBUG=fatal-criticals")
    endif()
  endif()
endif()
