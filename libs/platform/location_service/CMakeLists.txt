project(location_service)

if (NOT SKIP_QT_GUI AND PLATFORM_DESKTOP AND PLATFORM_LINUX)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GEOCLUE REQUIRED IMPORTED_TARGET libgeoclue-2.0)
  set(GEOCLUE_LOCATION_SERVICE true)
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
elseif(APPLE_LOCATION_SERVICE)
  target_compile_definitions(${PROJECT_NAME} PRIVATE "APPLE_LOCATION_SERVICE")
  target_link_libraries(${PROJECT_NAME} PRIVATE -framework\ CoreLocation)
endif()

if (BUILD_TESTING AND PLATFORM_DESKTOP)
  set(TEST_SRC position_filter_tests.cpp)
  if (GEOCLUE_LOCATION_SERVICE)
    list(APPEND TEST_SRC fake_geoclue_services.hpp geoclue_location_service_tests.cpp)
  endif()
  omim_add_test(location_service_tests ${TEST_SRC} NO_PLATFORM_INIT)
  target_link_libraries(location_service_tests PRIVATE location_service)
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
