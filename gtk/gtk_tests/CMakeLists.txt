project(gtk_tests)

set(SRC
  gui_thread_tests.cpp
  location_service_tests.cpp
  touch_input_tests.cpp
  ../location.cpp
  ../touch_input.cpp
)

set_source_files_properties(../location.cpp PROPERTIES COMPILE_OPTIONS -fno-finite-math-only)
omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE drape_frontend PkgConfig::GTK_APP_DEPS)
# Qt's "signals" keyword breaks GLib headers.
target_compile_definitions(${PROJECT_NAME} PRIVATE QT_NO_KEYWORDS)

# GClueSimple caches its backend selection: test the portal in a separate process.
add_test(NAME gtk_portal_tests COMMAND $<TARGET_FILE:gtk_tests> --filter=LocationService_AsyncLifecycle)
set_tests_properties(gtk_portal_tests PROPERTIES LABELS omim-test ENVIRONMENT "GTK_USE_PORTAL=1")

# Keep compositor-dependent integration tests separate from the display-free unit tests.
set(WINDOW_SRC
  window_lifecycle_tests.cpp
  ../about.cpp
  ../app.cpp
  ../gl_factory.cpp
  ../location.cpp
  ../main_window.cpp
  ../map_view.cpp
  ../touch_input.cpp
  "${ICONS_SRC}"
)
omim_add_test_target(gtk_window_tests "${WINDOW_SRC}" FALSE FALSE FALSE FALSE)
set_target_properties(gtk_window_tests PROPERTIES UNITY_BUILD OFF)
get_target_property(GTK_APP_DEFINITIONS organicmaps_gtk COMPILE_DEFINITIONS)
target_compile_definitions(gtk_window_tests PRIVATE ${GTK_APP_DEFINITIONS} OM_GTK_WINDOW_TESTS)
target_compile_options(gtk_window_tests PRIVATE -Wno-missing-field-initializers -Wno-unused-parameter)
target_link_libraries(gtk_window_tests PRIVATE map PkgConfig::GTK_APP_DEPS)
target_link_options(gtk_window_tests PRIVATE -Wl,--exclude-libs,ALL)

find_program(MUTTER mutter)
find_program(DBUS_RUN_SESSION dbus-run-session)
find_program(GDBUS gdbus)
if (MUTTER AND DBUS_RUN_SESSION AND GDBUS)
  add_test(NAME gtk_window_tests
    COMMAND "${DBUS_RUN_SESSION}" -- bash "${CMAKE_CURRENT_SOURCE_DIR}/run_window_tests.sh"
            $<TARGET_FILE:gtk_window_tests>
            --data_path=${OMIM_DATA_DIR}
            --user_resource_path=${OMIM_USER_RESOURCES_DIR}
            --writable_path=${CMAKE_BINARY_DIR}/test_writable/gtk_window_tests
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  )
  set_tests_properties(gtk_window_tests PROPERTIES LABELS gtk-display-test TIMEOUT 120)
endif()
