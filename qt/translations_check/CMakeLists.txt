project(qt_translations_check)

find_package(Qt5 COMPONENTS Gui LinguistTools)
if (NOT Qt5Gui_FOUND OR NOT Qt5LinguistTools_FOUND)
  message(STATUS "${PROJECT_NAME}: skipped, it needs Qt 5 Gui and LinguistTools")
  return()
endif()
include(OmimQtTranslations)

omim_add_executable(${PROJECT_NAME} main.cpp ../qt_common/translations.cpp ../qt_common/translations.hpp)
set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Gui platform qt_translations)
