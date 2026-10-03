project(desktop)

add_subdirectory(build_style)

QT6_ADD_RESOURCES(RES_SOURCES res/resources.qrc)

set(SRC
  about.cpp
  about.hpp
  bookmark_dialog.cpp
  bookmark_dialog.hpp
  bookmark_export.cpp
  bookmark_export.hpp
  create_feature_dialog.cpp
  create_feature_dialog.hpp
  draw_widget.cpp
  draw_widget.hpp
  editor_dialog.cpp
  editor_dialog.hpp
  html_processor.cpp
  html_processor.hpp
  info_dialog.cpp
  info_dialog.hpp
  main.cpp
  mainwindow.cpp
  mainwindow.hpp
  mwms_borders_selection.cpp
  mwms_borders_selection.hpp
  osm_auth_dialog.cpp
  osm_auth_dialog.hpp
  place_page_dialog_common.cpp
  place_page_dialog_common.hpp
  place_page_dialog_developer.cpp
  place_page_dialog_developer.hpp
  place_page_dialog_user.cpp
  place_page_dialog_user.hpp
  preferences_dialog.cpp
  preferences_dialog.hpp
  popup_menu_holder.cpp
  popup_menu_holder.hpp
  routing_settings_dialog.cpp
  routing_settings_dialog.hpp
  routing_turns_visualizer.cpp
  routing_turns_visualizer.hpp
  ruler.cpp
  ruler.hpp
  screenshoter.cpp
  screenshoter.hpp
  search_panel.cpp
  search_panel.hpp
  selection.hpp
  update_dialog.cpp
  update_dialog.hpp
)

omim_add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${RES_SOURCES} ${SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    generator  # For borders::LoadBorders
    qt_common
    map
    qt_designer
    gflags::gflags
    location_service
)

set(BUNDLE_NAME "OrganicMaps")
set(BUNDLE_DISPLAY_NAME "Organic Maps")
set(BUNDLE_ICON mac.icns)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${BUNDLE_NAME})

set(BUNDLE_EXECUTABLE ${BUNDLE_NAME})

set(BUNDLE_FOLDER ${CMAKE_BINARY_DIR}/${BUNDLE_NAME}.app)
set(RESOURCES_FOLDER ${BUNDLE_FOLDER}/Contents/Resources)
set(DATA_DIR ${OMIM_ROOT}/data)
set(RUNTIME_RESOURCES_FILE "${CMAKE_CURRENT_SOURCE_DIR}/runtime_resources.txt")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${RUNTIME_RESOURCES_FILE}")
file(STRINGS "${RUNTIME_RESOURCES_FILE}" RUNTIME_RESOURCES)

function(copy_resources)
  foreach(file ${ARGN})
    file(COPY "${DATA_DIR}/${file}" DESTINATION "${RESOURCES_FOLDER}")
  endforeach()
endfunction()

# The bundle resources are only read on macOS, the other platforms resolve the data directory at run time.
if (PLATFORM_MAC)
  copy_resources(${RUNTIME_RESOURCES})
  if (NOT WITH_SYSTEM_PROVIDED_3PARTY)
    copy_resources(icudt78l.dat)
  endif()
  file(COPY ${PROJECT_SOURCE_DIR}/res/mac/${BUNDLE_ICON} DESTINATION ${RESOURCES_FOLDER})
endif()

if (PLATFORM_WIN)
  set(OMIM_WINDOWS_ICON "${CMAKE_CURRENT_SOURCE_DIR}/res/windows/windows.ico")
  configure_file(res/windows/windows.rc.in "${CMAKE_CURRENT_BINARY_DIR}/windows.rc" @ONLY)
  target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/windows.rc")

  if (NOT OMIM_WINDOWS_DISTRIBUTION STREQUAL "development")
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE ON)
  endif()
endif()

if (PLATFORM_WIN AND NOT OMIM_WINDOWS_DISTRIBUTION STREQUAL "development")
  install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION . CONFIGURATIONS Release COMPONENT WindowsRuntime)
  if (NOT TARGET Qt6::windeployqt)
    message(FATAL_ERROR "The selected Qt installation has no Qt6::windeployqt target")
  endif()
  get_target_property(OMIM_WINDEPLOYQT_EXECUTABLE Qt6::windeployqt LOCATION)
  configure_file("${OMIM_ROOT}/cmake/windows/StageWindowsRuntime.cmake.in"
                 "${CMAKE_CURRENT_BINARY_DIR}/StageWindowsRuntime.cmake" @ONLY)
  install(CODE "include(\"${CMAKE_CURRENT_BINARY_DIR}/StageWindowsRuntime.cmake\")" COMPONENT WindowsRuntime)
else()
  install(TARGETS ${PROJECT_NAME} DESTINATION bin/)
  install(DIRECTORY ${OMIM_ROOT}/data DESTINATION share/organicmaps/)
endif()

if (PLATFORM_LINUX)
  install(FILES ${OMIM_ROOT}/packaging/app.organicmaps.desktop.metainfo.xml DESTINATION share/metainfo/)
  install(FILES ${OMIM_ROOT}/qt/res/linux/app.organicmaps.desktop.desktop DESTINATION share/applications/)
  install(FILES ${OMIM_ROOT}/qt/res/logo.png DESTINATION share/icons/hicolor/512x512/apps/ RENAME organicmaps.png)
endif()

if (PLATFORM_MAC)
  if (NOT APP_VERSION)
    set(BUNDLE_VERSION "UNKNOWN")
  else()
    set(BUNDLE_VERSION ${APP_VERSION})
  endif()

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/res/mac/Info.plist.in ${CMAKE_CURRENT_BINARY_DIR}/generated/Info.plist)

  set_target_properties(${PROJECT_NAME}
    PROPERTIES
      OUTPUT_NAME ${BUNDLE_NAME}
      MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/generated/Info.plist
  )
endif()

if (PLATFORM_MAC AND BUILD_STANDALONE)
  add_custom_command(TARGET desktop POST_BUILD
    COMMAND "${QT_PATH}/bin/macdeployqt" ${BUNDLE_FOLDER}
    COMMAND ${Python3_EXECUTABLE} "${OMIM_ROOT}/tools/macdeployqtfix/macdeployqtfix.py" -q -nl ${BUNDLE_FOLDER}/Contents/MacOS/${BUNDLE_NAME} ${QT_PATH}
    COMMAND echo "Fixing Qt dependencies finished."
    VERBATIM
    COMMENT "Fixing Qt dependencies for standalone desktop app"
  )
endif()

add_subdirectory(qt_common)

omim_add_test_subdirectory(qt_tests)
