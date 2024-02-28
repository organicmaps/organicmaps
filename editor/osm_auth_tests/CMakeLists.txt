project(osm_auth_tests)

set(SRC
  osm_auth_tests.cpp
  server_api_test.cpp
)

if (PLATFORM_MAC)
  append(SRC
    macos_helper.mm
  )
endif()

omim_add_test(${PROJECT_NAME} ${SRC} REQUIRE_QT)

target_link_libraries(${PROJECT_NAME} editor)
