project(desktop)

QT6_ADD_RESOURCES(RES_SOURCES res/resources.qrc)

set(SRC
  about.cpp
  about.hpp
  bookmark_dialog.cpp
  bookmark_dialog.hpp
  bookmark_export.cpp
  bookmark_export.hpp
  build_style/build_common.cpp
  build_style/build_common.h
  build_style/build_drules.cpp
  build_style/build_drules.h
  build_style/build_phone_pack.cpp
  build_style/build_phone_pack.h
  build_style/build_skins.cpp
  build_style/build_skins.h
  build_style/build_statistics.cpp
  build_style/build_statistics.h
  build_style/build_style.cpp
  build_style/build_style.h
  build_style/run_tests.cpp
  build_style/run_tests.h
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
  qml/forwarding_quickwidget.cpp
  qml/forwarding_quickwidget.hpp
)

omim_add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${RES_SOURCES} ${SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)

function(omim_compile_qt_catalog ts qm)
  get_filename_component(output_dir "${qm}" DIRECTORY)
  add_custom_command(
    OUTPUT "${qm}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${output_dir}"
    COMMAND ${CMAKE_COMMAND}
      -D "LRELEASE=$<TARGET_FILE:Qt6::lrelease>"
      -D "QT_VERSION=${Qt6Core_VERSION}"
      -D "TS=${ts}"
      -D "QM=${qm}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/strict_lrelease.cmake"
    DEPENDS Qt6::lrelease "${ts}" "${CMAKE_CURRENT_SOURCE_DIR}/strict_lrelease.cmake"
    VERBATIM
  )
endfunction()

file(GLOB_RECURSE QT_TRANSLATION_TS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/translations/*/organicmaps.ts")
find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
set(TWINE_DIR "${OMIM_ROOT}/tools/python/twine/python_twine")
file(GLOB_RECURSE TWINE_SOURCES CONFIGURE_DEPENDS "${TWINE_DIR}/twine/*.py")
set(QT_TRANSLATION_QM)
foreach(ts ${QT_TRANSLATION_TS})
  get_filename_component(lang_dir "${ts}" DIRECTORY)
  get_filename_component(lang "${lang_dir}" NAME)
  string(TOLOWER "${lang}" resource_locale)
  set(qm "${CMAKE_CURRENT_BINARY_DIR}/translations/organicmaps_${resource_locale}.qm")
  omim_compile_qt_catalog("${ts}" "${qm}")
  list(APPEND QT_TRANSLATION_QM "${qm}")

  # Type catalogs are build artifacts: their shared source already contains every consumer's keys.
  set(types_ts "${CMAKE_CURRENT_BINARY_DIR}/translations/types_${resource_locale}.ts")
  set(types_qm "${CMAKE_CURRENT_BINARY_DIR}/translations/organicmaps_types_${resource_locale}.qm")
  add_custom_command(
    OUTPUT "${types_ts}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/translations"
    COMMAND ${Python3_EXECUTABLE} "${TWINE_DIR}/twine_cli.py" generate-localization-file
      "${OMIM_ROOT}/data/strings/types_strings.txt" "${types_ts}"
      -f qt --include all -l "${lang}"
    DEPENDS "${OMIM_ROOT}/data/strings/types_strings.txt" "${TWINE_DIR}/twine_cli.py" ${TWINE_SOURCES}
    VERBATIM
  )
  omim_compile_qt_catalog("${types_ts}" "${types_qm}")
  list(APPEND QT_TRANSLATION_QM "${types_qm}")
endforeach()

add_custom_target(desktop_translations DEPENDS ${QT_TRANSLATION_QM})

get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
execute_process(
  COMMAND "${QT_QMAKE_EXECUTABLE}" -query QT_INSTALL_TRANSLATIONS
  OUTPUT_VARIABLE QT_BASE_TRANSLATIONS_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY
)
file(GLOB QT_BASE_TRANSLATIONS CONFIGURE_DEPENDS "${QT_BASE_TRANSLATIONS_DIR}/qtbase_*.qm")
if(NOT QT_BASE_TRANSLATIONS)
  message(FATAL_ERROR "Qt base translations are missing from ${QT_BASE_TRANSLATIONS_DIR}; install the Qt translations package")
endif()

function(omim_add_qt_translations target)
  add_dependencies(${target} desktop_translations)
  qt6_add_resources(${target} "${target}_translations"
    PREFIX "/translations"
    BASE "${PROJECT_BINARY_DIR}/translations"
    FILES ${QT_TRANSLATION_QM}
  )
  qt6_add_resources(${target} "${target}_qt_translations"
    PREFIX "/translations"
    BASE "${QT_BASE_TRANSLATIONS_DIR}"
    FILES ${QT_BASE_TRANSLATIONS}
  )
endfunction()

omim_add_qt_translations(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    generator  # For borders::LoadBorders
    qt_common
    map
    gflags::gflags
    location_service
)

qt_add_qml_module(${PROJECT_NAME}
    URI Map
    # QML_FILES cloud.qml
    # RESOURCE_PREFIX /qquickwidgetversuswindow_opengl
    # NO_RESOURCE_TARGET_PATH
    # DEPENDENCIES QtQuick
    # SOURCES draw_widget.cpp draw_widget.hpp
)

if (BUILD_DESIGNER)
  target_link_libraries(${PROJECT_NAME} PRIVATE generator)
  set(BUNDLE_NAME "OrganicMaps.Designer")
  set(BUNDLE_DISPLAY_NAME "Organic Maps Designer")
  set(BUNDLE_ICON designer.icns)
else()
  set(BUNDLE_NAME "OrganicMaps")
  set(BUNDLE_DISPLAY_NAME "Organic Maps")
  set(BUNDLE_ICON mac.icns)
endif()

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

if (PLATFORM_MAC AND BUILD_DESIGNER)
  execute_process(
    COMMAND cp -rf ${OMIM_ROOT}/data/symbols/mdpi/light/ ${OMIM_ROOT}/data/symbols/mdpi/design/
    COMMAND cp -rf ${OMIM_ROOT}/data/symbols/hdpi/light/ ${OMIM_ROOT}/data/symbols/hdpi/design/
    COMMAND cp -rf ${OMIM_ROOT}/data/symbols/xhdpi/light/ ${OMIM_ROOT}/data/symbols/xhdpi/design/
    COMMAND cp -rf ${OMIM_ROOT}/data/symbols/xxhdpi/light/ ${OMIM_ROOT}/data/symbols/xxhdpi/design/
    COMMAND cp -rf ${OMIM_ROOT}/data/symbols/6plus/light/ ${OMIM_ROOT}/data/symbols/6plus/design/
    COMMAND cp -rf ${OMIM_ROOT}/data/symbols/xxxhdpi/light/ ${OMIM_ROOT}/data/symbols/xxxhdpi/design/
    COMMAND cp -f ${OMIM_ROOT}/data/colors.txt ${OMIM_ROOT}/data/colors_design.txt
    COMMAND cp -f ${OMIM_ROOT}/data/patterns.txt ${OMIM_ROOT}/data/patterns_design.txt
  )

  copy_resources(
    colors_design.txt
    drules_design.bin
    mapcss-dynamic.txt
    mapcss-mapping.csv
    patterns_design.txt
    symbols/mdpi/design
    symbols/hdpi/design
    symbols/xhdpi/design
    symbols/xxhdpi/design
    symbols/xxxhdpi/design
    symbols/6plus/design
  )

  execute_process(
    COMMAND cp ${OMIM_ROOT}/tools/python/recalculate_geom_index.py ${RESOURCES_FOLDER}
    COMMAND cp ${OMIM_ROOT}/tools/python/generate_styles_override.py ${RESOURCES_FOLDER}
    COMMAND cp -rf ${OMIM_ROOT}/tools/kothic ${RESOURCES_FOLDER}/kothic/
    COMMAND cp -f ${OMIM_ROOT}/tools/python/stylesheet/drules_info.py ${RESOURCES_FOLDER}/kothic/src/
    COMMAND cp -rf ${OMIM_ROOT}/tools/python/stylesheet/ ${RESOURCES_FOLDER}/kothic/src/
  )

  # Generate DMG
  install(DIRECTORY ${DATA_DIR}/styles DESTINATION .)
  set(BUNDLES
    ${CMAKE_BINARY_DIR}/generator_tool.app
    ${CMAKE_BINARY_DIR}/skin_generator_tool.app
    ${CMAKE_BINARY_DIR}/style_tests.app
  )
  install(CODE "
    foreach(BUNDLE ${BUNDLE_FOLDER} ${BUNDLES})
      execute_process(
        COMMAND \"${QT_PATH}/bin/macdeployqt\" \"\${BUNDLE}\"
      )
    endforeach()
    foreach(BUNDLE ${BUNDLES})
      execute_process(
        COMMAND cp -rf \"\${BUNDLE}\" \"${RESOURCES_FOLDER}\"
      )
    endforeach()
  ")
  install(TARGETS ${PROJECT_NAME} DESTINATION .)

  set(CPACK_GENERATOR DragNDrop)
  set(CPACK_DMG_FORMAT UDZO)
  set(CPACK_DMG_VOLUME_NAME ${BUNDLE_NAME})
  set(CPACK_PACKAGE_ICON ${PROJECT_SOURCE_DIR}/res/mac/designer.icns)
  include(CPack)
elseif (PLATFORM_MAC AND BUILD_STANDALONE)
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
