project(organicmaps_gtk)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK_APP_DEPS REQUIRED IMPORTED_TARGET gtk4>=4.12 libadwaita-1>=1.6 libgeoclue-2.0)
pkg_get_variable(GLIB_COMPILE_RESOURCES gio-2.0 glib_compile_resources)
if (NOT GLIB_COMPILE_RESOURCES)
  message(FATAL_ERROR "glib-compile-resources is not found")
endif()

set(SRC
  about.cpp
  about.hpp
  app.cpp
  app.hpp
  gl_factory.cpp
  gl_factory.hpp
  location.cpp
  location.hpp
  main.cpp
  main_window.cpp
  main_window.hpp
  map_view.cpp
  map_view.hpp
  style.hpp
  touch_input.cpp
  touch_input.hpp
)

# Icons are compiled into the executable.
set(ICONS_XML "${CMAKE_CURRENT_SOURCE_DIR}/res/icons.gresource.xml")
set(ICONS_SRC "${CMAKE_CURRENT_BINARY_DIR}/icons_resources.c")
file(GLOB ICON_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/res/icons/*")
add_custom_command(
  OUTPUT "${ICONS_SRC}"
  COMMAND "${GLIB_COMPILE_RESOURCES}" --generate-source --target=${ICONS_SRC}
          --sourcedir=${CMAKE_CURRENT_SOURCE_DIR}/res/icons ${ICONS_XML}
  DEPENDS "${ICONS_XML}" ${ICON_FILES}
  VERBATIM
)

omim_add_executable(${PROJECT_NAME} ${SRC} "${ICONS_SRC}")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME organicmaps-gtk UNITY_BUILD OFF)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  QT_NO_KEYWORDS
  GDK_VERSION_MIN_REQUIRED=GDK_VERSION_4_12
  GDK_VERSION_MAX_ALLOWED=GDK_VERSION_4_12
  ADW_VERSION_MIN_REQUIRED=ADW_VERSION_1_6
  ADW_VERSION_MAX_ALLOWED=ADW_VERSION_1_6
)
target_compile_options(${PROJECT_NAME} PRIVATE -Wno-missing-field-initializers -Wno-unused-parameter)
target_link_libraries(${PROJECT_NAME} PRIVATE map PkgConfig::GTK_APP_DEPS)
# The core links its own static FreeType and HarfBuzz: don't export them, or they override the
# system ones used by Pango and cairo (without PNG support color emoji are not drawn).
target_link_options(${PROJECT_NAME} PRIVATE -Wl,--exclude-libs,ALL)

omim_add_test_subdirectory(gtk_tests)

install(TARGETS ${PROJECT_NAME} DESTINATION bin/)
install(DIRECTORY ${OMIM_ROOT}/data DESTINATION share/organicmaps/)
install(FILES res/app.organicmaps.gtk.desktop DESTINATION share/applications/)
install(FILES ${OMIM_ROOT}/qt/res/logo.png DESTINATION share/icons/hicolor/512x512/apps/
        RENAME app.organicmaps.gtk.png)
