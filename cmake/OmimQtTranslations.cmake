include_guard(GLOBAL)

if (OMIM_QT_VERSION STREQUAL "5")
  find_package(Qt5 REQUIRED COMPONENTS LinguistTools)
endif()
set(QT_TRANSLATIONS_DIR "${CMAKE_BINARY_DIR}/qt/translations")

function(omim_compile_qt_catalog ts qm)
  get_filename_component(output_dir "${qm}" DIRECTORY)
  add_custom_command(
    OUTPUT "${qm}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${output_dir}"
    COMMAND ${CMAKE_COMMAND}
      -D "LRELEASE=$<TARGET_FILE:Qt${OMIM_QT_VERSION}::lrelease>"
      -D "QT_VERSION=${Qt${OMIM_QT_VERSION}Core_VERSION}"
      -D "TS=${ts}"
      -D "QM=${qm}"
      -P "${OMIM_ROOT}/qt/strict_lrelease.cmake"
    DEPENDS Qt${OMIM_QT_VERSION}::lrelease "${ts}" "${OMIM_ROOT}/qt/strict_lrelease.cmake"
    VERBATIM
  )
endfunction()

file(GLOB_RECURSE QT_TRANSLATION_TS CONFIGURE_DEPENDS "${OMIM_ROOT}/qt/translations/*/organicmaps.ts")
set(TWINE_DIR "${OMIM_ROOT}/tools/python/twine/python_twine")
file(GLOB_RECURSE TWINE_SOURCES CONFIGURE_DEPENDS "${TWINE_DIR}/twine/*.py")
set(QT_TRANSLATION_QM)
foreach(ts ${QT_TRANSLATION_TS})
  get_filename_component(lang_dir "${ts}" DIRECTORY)
  get_filename_component(lang "${lang_dir}" NAME)
  string(TOLOWER "${lang}" resource_locale)
  set(qm "${QT_TRANSLATIONS_DIR}/organicmaps_${resource_locale}.qm")
  omim_compile_qt_catalog("${ts}" "${qm}")
  list(APPEND QT_TRANSLATION_QM "${qm}")

  # Type catalogs are build artifacts: their shared source already contains every consumer's keys.
  set(types_ts "${QT_TRANSLATIONS_DIR}/types_${resource_locale}.ts")
  set(types_qm "${QT_TRANSLATIONS_DIR}/organicmaps_types_${resource_locale}.qm")
  add_custom_command(
    OUTPUT "${types_ts}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${QT_TRANSLATIONS_DIR}"
    COMMAND ${Python3_EXECUTABLE} "${TWINE_DIR}/twine_cli.py" generate-localization-file
      "${OMIM_ROOT}/data/strings/types_strings.txt" "${types_ts}"
      -f qt --include all -l "${lang}"
    DEPENDS "${OMIM_ROOT}/data/strings/types_strings.txt" "${TWINE_DIR}/twine_cli.py" ${TWINE_SOURCES}
    VERBATIM
  )
  omim_compile_qt_catalog("${types_ts}" "${types_qm}")
  list(APPEND QT_TRANSLATION_QM "${types_qm}")
endforeach()

if (OMIM_QT_VERSION STREQUAL "6")
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
endif()

add_library(qt_translations OBJECT EXCLUDE_FROM_ALL)
target_link_libraries(qt_translations PRIVATE Qt${OMIM_QT_VERSION}::Core)
if (OMIM_QT_VERSION STREQUAL "5")
  # Qt 5.6 can't add resources to a target directly.
  set(files "")
  foreach(qm ${QT_TRANSLATION_QM})
    get_filename_component(name "${qm}" NAME)
    string(APPEND files "    <file>${name}</file>\n")
  endforeach()
  set(qrc "${QT_TRANSLATIONS_DIR}/translations.qrc")
  file(CONFIGURE OUTPUT "${qrc}"
    CONTENT "<RCC>\n  <qresource prefix=\"/translations\">\n${files}  </qresource>\n</RCC>\n" @ONLY)
  qt5_add_resources(QT_TRANSLATION_SOURCES "${qrc}")
  target_sources(qt_translations PRIVATE ${QT_TRANSLATION_SOURCES})
else()
  qt6_add_resources(qt_translations "translations"
    PREFIX "/translations"
    BASE "${QT_TRANSLATIONS_DIR}"
    FILES ${QT_TRANSLATION_QM}
  )
  qt6_add_resources(qt_translations "qt_base_translations"
    PREFIX "/translations"
    BASE "${QT_BASE_TRANSLATIONS_DIR}"
    FILES ${QT_BASE_TRANSLATIONS}
  )
endif()
