project(gtk_tests)

set(SRC
  gui_thread_tests.cpp
  location_service_tests.cpp
  location_tests.cpp
  touch_input_tests.cpp
  ../location.cpp
  ../touch_input.cpp
)

omim_add_test(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE drape_frontend PkgConfig::GTK_APP_DEPS)
# Qt's "signals" keyword breaks GLib headers.
target_compile_definitions(${PROJECT_NAME} PRIVATE QT_NO_KEYWORDS)
