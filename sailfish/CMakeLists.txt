project(organicmaps_sailfish)

option(SAILFISH_HARBOUR "Build the Jolla Harbour package: harbour- name, no Sailjail profile and no voice" OFF)
# Aurora OS is Qt 5.6 + Silica like Sailfish, but ships libauroraapp.so.2 in place
# of the deprecated libsailfishapp.so.1, different app isolation and packaging.
option(AURORA "Build the Aurora OS package (uses auroraapp instead of sailfishapp)" OFF)

set(APP_ORG app.organicmaps)
# The application name: QGuiApplication::applicationName() and the "qml/<name>.qml" entry point.
set(APP_QML_NAME organicmaps)
# The launcher's openUrl and the notification actions.
set(DBUS_SERVICE app.organicmaps.organicmaps)
set(DBUS_PATH /app/organicmaps)

# appdir resolves "/usr/share/<org>.<app>", the same layout the desktop and RPM use.
if (AURORA)
  set(APP_NAME ${APP_ORG}.${APP_QML_NAME})
  set(APP_LIB auroraapp)
  set(APP_DESKTOP_IN organicmaps-aurora.desktop.in)
elseif (SAILFISH_HARBOUR)
  set(APP_NAME harbour-organicmaps)
  set(APP_LIB sailfishapp)
  set(APP_DESKTOP_IN organicmaps.desktop.in)
else()
  set(APP_NAME organicmaps)
  set(APP_LIB sailfishapp)
  set(APP_DESKTOP_IN organicmaps.desktop.in)
endif()
set(APP_DATA_DIR share/${APP_NAME})

find_package(Qt5 COMPONENTS REQUIRED DBus Gui Multimedia Network Qml Quick Sensors)
find_package(PkgConfig REQUIRED)
pkg_check_modules(APPLIB REQUIRED ${APP_LIB})
# The Sailfish/Aurora clock format, see Is24HourClock().
pkg_check_modules(MLITE REQUIRED mlite5)

# Aurora OS ships no speech engine and the app sandbox forbids running external programs, so turn
# notifications can be synthesized in-process with Piper. Voice is optional: pass VOICE_LIBS_DIR
# (the Aurora Conan packages piper, piper-phonemize, onnxruntime, espeak-ng, spdlog, fmt unpacked
# with include/ and lib/) to enable it. Without it the app builds and runs without spoken
# instructions. Voice models and the espeak-ng data are shipped by a separate package.
#
# Note: the prebuilt onnxruntime/XNNPACK libraries require an ARMv8.2 CPU (dot product); on an
# ARMv8.0 device they crash with SIGILL, so build without VOICE_LIBS_DIR there.
set(VOICE_LIBS_DIR "" CACHE PATH "Aurora Conan voice libraries directory (include/ + lib/); enables voice")
if (AURORA AND VOICE_LIBS_DIR)
  set(AURORA_VOICE ON)
else()
  set(AURORA_VOICE OFF)
endif()

set(SRC
  app.cpp
  app_info.cpp
  app_info.hpp
  app_settings.cpp
  app_settings.hpp
  bookmark_editor.cpp
  bookmark_editor.hpp
  bookmarks_io.cpp
  bookmarks_io.hpp
  bookmarks_model.cpp
  bookmarks_model.hpp
  bundled_html.cpp
  bundled_html.hpp
  countries_model.cpp
  countries_model.hpp
  elevation.cpp
  elevation.hpp
  file_log.cpp
  file_log.hpp
  framework_access.hpp
  helpers.cpp
  helpers.hpp
  localization.cpp
  map_item.cpp
  map_item.hpp
  maps_storage.cpp
  maps_storage.hpp
  opening_hours_editor.cpp
  opening_hours_editor.hpp
  osm_account.cpp
  osm_account.hpp
  place_editor.cpp
  place_editor.hpp
  place_page.cpp
  place_page.hpp
  routing.cpp
  routing.hpp
  search_model.cpp
  search_model.hpp
  url_handler.cpp
  url_handler.hpp
  voice_guide.cpp
  voice_guide.hpp
  ${OMIM_ROOT}/qt/qt_common/qtoglcontext.cpp
  ${OMIM_ROOT}/qt/qt_common/qtoglcontext.hpp
  ${OMIM_ROOT}/qt/qt_common/qtoglcontextfactory.cpp
  ${OMIM_ROOT}/qt/qt_common/qtoglcontextfactory.hpp
)

add_library(${PROJECT_NAME}_lib SHARED ${SRC})
set_target_properties(${PROJECT_NAME}_lib PROPERTIES OUTPUT_NAME organicmaps AUTOMOC ON)
target_compile_options(${PROJECT_NAME}_lib PRIVATE ${OMIM_WARNING_FLAGS})
target_compile_definitions(${PROJECT_NAME}_lib PRIVATE "SAILFISH_APP_NAME=\"${APP_QML_NAME}\""
                           "SAILFISH_DBUS_SERVICE=\"${DBUS_SERVICE}\"" "SAILFISH_DBUS_PATH=\"${DBUS_PATH}\""
                           $<$<BOOL:${SAILFISH_HARBOUR}>:OMIM_SAILFISH_HARBOUR>
                           $<$<BOOL:${AURORA}>:OMIM_AURORA>
                           $<$<BOOL:${AURORA_VOICE}>:OMIM_AURORA_VOICE>)
# Keep the bundled 3party libraries (FreeType, ICU, ...) from interposing the system copies Qt uses.
target_link_options(${PROJECT_NAME}_lib PRIVATE -Wl,--exclude-libs,ALL)
target_include_directories(${PROJECT_NAME}_lib PRIVATE ${APPLIB_INCLUDE_DIRS} ${MLITE_INCLUDE_DIRS})
if (AURORA_VOICE)
  # onnxruntime headers are included without a subdirectory prefix by piper.hpp.
  target_include_directories(${PROJECT_NAME}_lib PRIVATE
    "${VOICE_LIBS_DIR}/include" "${VOICE_LIBS_DIR}/include/onnxruntime")
endif()
target_link_libraries(${PROJECT_NAME}_lib
  PRIVATE
    Boost::headers
    map
    location_service
    Qt5::DBus
    Qt5::Gui
    Qt5::Multimedia
    Qt5::Network
    Qt5::Qml
    Qt5::Quick
    Qt5::Sensors
    ${APPLIB_LIBRARIES}
    ${MLITE_LIBRARIES}
)
if (AURORA_VOICE)
  # Piper and its transitive dependencies from the Aurora Conan packages.
  target_link_directories(${PROJECT_NAME}_lib PRIVATE "${VOICE_LIBS_DIR}/lib")
  target_link_libraries(${PROJECT_NAME}_lib PRIVATE piper piper_phonemize onnxruntime espeak-ng spdlog fmt)
endif()

add_executable(${PROJECT_NAME} main.cpp)
set_target_properties(${PROJECT_NAME} PROPERTIES
  OUTPUT_NAME ${APP_NAME}
  INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${APP_DATA_DIR}/lib"
)
# The Harbour validator only accepts DT_RPATH, not the DT_RUNPATH of the new dtags.
# --export-dynamic puts main() in the dynamic symbol table for the booster, see main.cpp.
target_link_options(${PROJECT_NAME} PRIVATE -Wl,--disable-new-dtags -Wl,--export-dynamic)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_lib)

file(STRINGS "${OMIM_ROOT}/qt/runtime_resources.txt" RUNTIME_RESOURCES)
# Voice phrases, the FAQ and the opening hours examples, which the desktop app doesn't install.
list(APPEND RUNTIME_RESOURCES icudt78l.dat sound-strings faq.html opening_hours_how_to_edit.html)
foreach(resource ${RUNTIME_RESOURCES})
  if (IS_DIRECTORY "${OMIM_ROOT}/data/${resource}")
    install(DIRECTORY "${OMIM_ROOT}/data/${resource}" DESTINATION ${APP_DATA_DIR}/data)
  else()
    install(FILES "${OMIM_ROOT}/data/${resource}" DESTINATION ${APP_DATA_DIR}/data)
  endif()
endforeach()

# Read at runtime by localization.cpp.
install(FILES "${OMIM_ROOT}/data/strings/types_strings.txt" "${OMIM_ROOT}/data/strings/strings.txt"
        DESTINATION ${APP_DATA_DIR}/data/strings)
install(FILES strings.txt DESTINATION ${APP_DATA_DIR}/data/strings RENAME sailfish_strings.txt)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(TARGETS ${PROJECT_NAME}_lib LIBRARY DESTINATION ${APP_DATA_DIR}/lib)
if (AURORA_VOICE)
  # Piper and its dependencies, resolved from the private lib directory through the RPATH.
  install(DIRECTORY "${VOICE_LIBS_DIR}/lib/" DESTINATION ${APP_DATA_DIR}/lib)
endif()
install(DIRECTORY sounds DESTINATION ${APP_DATA_DIR})
install(DIRECTORY qml DESTINATION ${APP_DATA_DIR} PATTERN organicmaps.qml EXCLUDE)
install(FILES qml/organicmaps.qml DESTINATION ${APP_DATA_DIR}/qml RENAME ${APP_QML_NAME}.qml)
# SVG icons converted from the Android vector drawables.
set(SVG_ICONS_DIR "${CMAKE_CURRENT_BINARY_DIR}/icons")
add_custom_command(
  OUTPUT "${SVG_ICONS_DIR}/.stamp"
  # Starts afresh, so that icons no longer converted aren't installed.
  COMMAND ${CMAKE_COMMAND} -E remove_directory "${SVG_ICONS_DIR}"
  COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tools/android_vector_to_svg.py" "${SVG_ICONS_DIR}"
  COMMAND ${CMAKE_COMMAND} -E touch "${SVG_ICONS_DIR}/.stamp"
  DEPENDS tools/android_vector_to_svg.py
  COMMENT "Converting Android vector drawables to SVG icons"
)
add_custom_target(${PROJECT_NAME}_icons DEPENDS "${SVG_ICONS_DIR}/.stamp")
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_icons)
install(DIRECTORY "${SVG_ICONS_DIR}/" DESTINATION ${APP_DATA_DIR}/icons PATTERN .stamp EXCLUDE)
# Router and route point buttons, which are bitmaps on Android too.
set(ANDROID_RES "${OMIM_ROOT}/android/app/src/main/res")
install(FILES
  "${ANDROID_RES}/drawable-xxhdpi/ic_car.webp" "${ANDROID_RES}/drawable-xxhdpi/ic_pedestrian.webp"
  "${ANDROID_RES}/drawable-xxhdpi/ic_transit.webp" "${ANDROID_RES}/drawable-xxhdpi/ic_bike.webp"
  "${ANDROID_RES}/drawable-xxhdpi/ic_route_from.webp" "${ANDROID_RES}/drawable-xxhdpi/ic_route_to.webp"
  "${ANDROID_RES}/drawable-xxhdpi/ic_route_via.webp"
  "${ANDROID_RES}/drawable-xxhdpi/ic_avoid_tolls.webp" "${ANDROID_RES}/drawable-xxhdpi/ic_avoid_unpaved.webp"
  "${ANDROID_RES}/drawable-xxhdpi/ic_avoid_ferry.webp"
  "${ANDROID_RES}/drawable/route_point_start.png" "${ANDROID_RES}/drawable/route_point_finish.png"
  DESTINATION ${APP_DATA_DIR}/icons/routing)
set(ANDROID_SDK_RES "${OMIM_ROOT}/android/sdk/src/main/res")
install(FILES
  "${ANDROID_SDK_RES}/drawable-xxxhdpi/ic_20px_route_planning_walk.webp"
  "${ANDROID_SDK_RES}/drawable-xxxhdpi/ic_20px_route_planning_metro.webp"
  "${ANDROID_SDK_RES}/drawable-xxxhdpi/ic_20px_route_planning_train.webp"
  "${ANDROID_SDK_RES}/drawable-xxxhdpi/ic_20px_route_planning_lightrail.webp"
  "${ANDROID_SDK_RES}/drawable-xxxhdpi/ic_20px_route_planning_monorail.webp"
  DESTINATION ${APP_DATA_DIR}/icons/routing)
set(TURNS straight right right_sharp right_slight left left_sharp left_slight uleft uright finish)
list(TRANSFORM TURNS PREPEND "${ANDROID_SDK_RES}/drawable-xxxhdpi/ic_turn_")
list(TRANSFORM TURNS APPEND ".webp")
install(FILES ${TURNS}
  "${ANDROID_SDK_RES}/drawable-xxxhdpi/ic_exit_highway_to_left.webp"
  "${ANDROID_SDK_RES}/drawable-xxxhdpi/ic_exit_highway_to_right.webp"
  DESTINATION ${APP_DATA_DIR}/icons/navigation)
# Icons that are bitmaps on Android too.
install(FILES "${ANDROID_RES}/drawable/ic_openstreetmap_color.webp" "${ANDROID_RES}/drawable/ic_crowdfunding.png"
        DESTINATION ${APP_DATA_DIR}/icons/help)
# The Wikipedia logos are traced from the iOS ones.
install(FILES "${ANDROID_RES}/drawable-xxhdpi/ic_direction_pagepreview.webp"
              "${ANDROID_RES}/drawable-sw600dp-xxhdpi/ic_direction_fullscreen.png"
              "${ANDROID_RES}/drawable-xxxhdpi/ic_entrance.webp"
              icons/placepage/ic_wiki.svg icons/placepage/ic_wikimedia_commons.svg
        DESTINATION ${APP_DATA_DIR}/icons/placepage)
configure_file(${APP_DESKTOP_IN} ${APP_NAME}.desktop @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${APP_NAME}.desktop" DESTINATION share/applications)
if (NOT SAILFISH_HARBOUR AND NOT AURORA)
  # Sailjail applies it to the app by name, see the file. Aurora isolates apps via the desktop [X-Application] section.
  install(FILES organicmaps.profile DESTINATION /etc/sailjail/permissions)
endif()

# The Apache-2.0 code license, the NOTICE and the Organic Maps binary data license must ship with the app.
# LICENSE is a symlink in the repo; install the real license text as a regular file (the validator rejects symlinks).
install(FILES "${OMIM_ROOT}/LICENSES/Apache-2.0.txt" DESTINATION ${APP_DATA_DIR} RENAME LICENSE)
install(FILES "${OMIM_ROOT}/NOTICE" "${OMIM_ROOT}/DATA_LICENSE.txt" DESTINATION ${APP_DATA_DIR})
foreach(size 86 108 128 172)
  install(FILES icons/${size}x${size}/organicmaps.png DESTINATION share/icons/hicolor/${size}x${size}/apps
          RENAME ${APP_NAME}.png)
endforeach()
