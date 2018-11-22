project(desktop)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

QT5_ADD_RESOURCES(RES_SOURCES res/resources.qrc)

set_property(SOURCE qrc_resources.cpp PROPERTY SKIP_AUTOGEN ON)

include_directories(
  ${OMIM_ROOT}/3party/glm
  ${OMIM_ROOT}/3party/gflags/src
  ${OMIM_ROOT}/3party/jansson/src
)

set(
  SRC
  about.cpp
  about.hpp
  bookmark_dialog.cpp
  bookmark_dialog.hpp
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
  info_dialog.cpp
  info_dialog.hpp
  main.cpp
  mainwindow.cpp
  mainwindow.hpp
  osm_auth_dialog.cpp
  osm_auth_dialog.hpp
  place_page_dialog.cpp
  place_page_dialog.hpp
  preferences_dialog.cpp
  preferences_dialog.hpp
  search_panel.cpp
  search_panel.hpp
  update_dialog.cpp
  update_dialog.hpp
)

omim_add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${RES_SOURCES} ${SRC})

omim_link_libraries(
  ${PROJECT_NAME}
  qt_common
  map
  drape_frontend
  shaders
  routing
  search
  storage
  mwm_diff
  bsdiff
  tracking
  traffic
  routing_common
  transit
  descriptions
  ugc
  drape
  partners_api
  local_ads
  kml
  editor
  indexer
  metrics
  platform
  geometry
  coding
  base
  freetype
  expat
  gflags
  icu
  agg
  jansson
  protobuf
  stats_client
  minizip
  succinct
  pugixml
  oauthcpp
  opening_hours
  stb_image
  sdf_image
  ${Qt5Gui_LIBRARIES}
  ${Qt5Network_LIBRARIES}
  ${Qt5Widgets_LIBRARIES}
  ${LIBZ}
)

link_opengl(${PROJECT_NAME})
link_qt5_core(${PROJECT_NAME})

if (BUILD_DESIGNER)
  set(BUNDLE_NAME "MAPS.ME.Designer")
  set(BUNDLE_DISPLAY_NAME "MAPS.ME Designer")
  set(BUNDLE_ICON designer.icns)
else()
  set(BUNDLE_NAME "MAPS.ME")
  set(BUNDLE_DISPLAY_NAME "MAPS.ME Desktop")
  set(BUNDLE_ICON mac.icns)
endif()

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES
  OUTPUT_NAME ${BUNDLE_NAME}
)

set(BUNDLE_EXECUTABLE ${BUNDLE_NAME})

set(BUNDLE_FOLDER ${CMAKE_BINARY_DIR}/${BUNDLE_NAME}.app)
set(RESOURCES_FOLDER ${BUNDLE_FOLDER}/Contents/Resources)
set(DATA_DIR ${OMIM_ROOT}/data)

execute_process(
  COMMAND mkdir -p ${RESOURCES_FOLDER}/shaders_compiler
  COMMAND cp ${PROJECT_SOURCE_DIR}/res/mac.icns ${RESOURCES_FOLDER}
  COMMAND cp ${PROJECT_SOURCE_DIR}/res/designer.icns ${RESOURCES_FOLDER}
)

function(copy_resources)
  foreach(file ${ARGN})
    execute_process(
      COMMAND cp -r ${DATA_DIR}/${file} ${RESOURCES_FOLDER}
    )
  endforeach()
endfunction()

copy_resources(
  countries-strings
  resources-default
  resources-mdpi_clear
  cuisine-strings
  taxi_places
  eula.html

  categories.txt
  categories_cuisines.txt
  classificator.txt
  colors.txt
  copyright.html
  countries.txt
  countries_obsolete.txt
  drules_proto_clear.bin
  drules_proto_dark.bin
  drules_proto_vehicle_clear.bin
  drules_proto_vehicle_dark.bin
  editor.config
  fonts_blacklist.txt
  fonts_whitelist.txt
  packed_polygons.bin
  packed_polygons_obsolete.bin
  patterns.txt
  types.txt
  unicode_blocks.txt
  World.mwm
  WorldCoasts.mwm
  WorldCoasts_obsolete.mwm
  icudt57l.dat
  local_ads_symbols.txt

  01_dejavusans.ttf
  02_droidsans-fallback.ttf
  03_jomolhari-id-a3d.ttf
  04_padauk.ttf
  05_khmeros.ttf
  06_code2000.ttf
  07_roboto_medium.ttf
)

if (PLATFORM_MAC)
  execute_process(
    COMMAND cp -r ${OMIM_ROOT}/tools/shaders_compiler/macos ${RESOURCES_FOLDER}/shaders_compiler
  )
elseif(PLATFORM_LINUX)
  execute_process(
    COMMAND cp -r ${OMIM_ROOT}/tools/shaders_compiler/linux ${RESOURCES_FOLDER}/shaders_compiler
  )
endif()

if (PLATFORM_MAC)
  target_link_libraries(
    ${PROJECT_NAME}
    "-framework Foundation"
    "-framework CoreLocation"
    "-framework CoreWLAN"
    "-framework SystemConfiguration"
    "-framework CFNetwork"
  )

  if (NOT APP_VERSION)
    set(BUNDLE_VERSION "UNKNOWN")
  else()
    set(BUNDLE_VERSION ${APP_VERSION})
  endif()

  configure_file(${PROJECT_SOURCE_DIR}/res/Info.plist.in ${PROJECT_SOURCE_DIR}/res/Info.plist)

  set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    OUTPUT_NAME ${BUNDLE_NAME}
    MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/res/Info.plist
  )
endif()

if (BUILD_DESIGNER)
  execute_process(
    COMMAND cp -rf ${OMIM_ROOT}/data/resources-mdpi_clear/ ${OMIM_ROOT}/data/resources-mdpi_design/
    COMMAND cp -rf ${OMIM_ROOT}/data/resources-hdpi_clear/ ${OMIM_ROOT}/data/resources-hdpi_design/
    COMMAND cp -rf ${OMIM_ROOT}/data/resources-xhdpi_clear/ ${OMIM_ROOT}/data/resources-xhdpi_design/
    COMMAND cp -rf ${OMIM_ROOT}/data/resources-xxhdpi_clear/ ${OMIM_ROOT}/data/resources-xxhdpi_design/
    COMMAND cp -rf ${OMIM_ROOT}/data/resources-6plus_clear/ ${OMIM_ROOT}/data/resources-6plus_design/
    COMMAND cp -rf ${OMIM_ROOT}/data/resources-xxxhdpi_clear/ ${OMIM_ROOT}/data/resources-xxxhdpi_design/
    COMMAND cp -f ${OMIM_ROOT}/data/drules_proto_clear.bin ${OMIM_ROOT}/data/drules_proto_design.bin
    COMMAND cp -f ${OMIM_ROOT}/data/colors.txt ${OMIM_ROOT}/data/colors_design.txt
    COMMAND cp -f ${OMIM_ROOT}/data/patterns.txt ${OMIM_ROOT}/data/patterns_design.txt
  )

  copy_resources(
    colors_design.txt
    drules_proto_design.bin
    mapcss-dynamic.txt
    mapcss-mapping.csv
    patterns_design.txt
    resources-mdpi_design
    resources-hdpi_design
    resources-xhdpi_design
    resources-xxhdpi_design
    resources-xxxhdpi_design
    resources-6plus_design
  )

  execute_process(
    COMMAND cp ${OMIM_ROOT}/tools/python/recalculate_geom_index.py ${RESOURCES_FOLDER}
    COMMAND cp ${OMIM_ROOT}/tools/python/generate_styles_override.py ${RESOURCES_FOLDER}
    COMMAND cp -rf ${OMIM_ROOT}/tools/kothic ${RESOURCES_FOLDER}/kothic/
    COMMAND cp -f ${OMIM_ROOT}/tools/python/stylesheet/drules_info.py ${RESOURCES_FOLDER}/kothic/src/
    COMMAND cp -rf ${OMIM_ROOT}/tools/python/stylesheet/ ${RESOURCES_FOLDER}/kothic/src/
    COMMAND cp -f ${OMIM_ROOT}/3party/protobuf/protobuf-3.3.0-py2.7.egg ${RESOURCES_FOLDER}/kothic/
  )

  # Generate DMG
  install(DIRECTORY ${DATA_DIR}/styles DESTINATION .)
  set(
    BUNDLES
    ${CMAKE_BINARY_DIR}/generator_tool.app
    ${CMAKE_BINARY_DIR}/skin_generator_tool.app
    ${CMAKE_BINARY_DIR}/style_tests.app
  )
  install(CODE "
    foreach(BUNDLE ${BUNDLE_FOLDER} ${BUNDLES})
      execute_process(
        COMMAND \"${QT_PATH}/bin/macdeployqt\" \"\${BUNDLE}\"
        COMMAND python \"${OMIM_ROOT}/tools/macdeployqtfix/macdeployqtfix.py\" -q -nl \"\${BUNDLE}\" \"${QT_PATH}\"
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
  set(CPACK_PACKAGE_ICON ${PROJECT_SOURCE_DIR}/res/designer.icns)
  include(CPack)
endif()

add_subdirectory(qt_common)
