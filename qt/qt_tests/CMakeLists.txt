omim_add_test(update_dialog_tests
  update_dialog_tests.cpp
  ../update_dialog.cpp
)

target_link_libraries(update_dialog_tests
  PRIVATE
    qt_common
    map
    gflags::gflags
    location_service
)

set_target_properties(update_dialog_tests PROPERTIES AUTOMOC ON)
omim_add_qt_translations(update_dialog_tests)
set_tests_properties(update_dialog_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

omim_add_test(bookmark_export_tests NO_PLATFORM_INIT
  bookmark_export_tests.cpp
  ../bookmark_export.cpp
)
target_link_libraries(bookmark_export_tests PRIVATE Qt6::Core)

omim_add_test(translations_tests NO_PLATFORM_INIT translations_tests.cpp)
target_link_libraries(translations_tests PRIVATE qt_common)
omim_add_qt_translations(translations_tests)
set_tests_properties(translations_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_test(NAME strict_lrelease_tests
  COMMAND ${CMAKE_COMMAND}
    -D "LRELEASE=$<TARGET_FILE:Qt6::lrelease>"
    -D "QT_VERSION=${Qt6Core_VERSION}"
    -D "WRAPPER=${CMAKE_CURRENT_SOURCE_DIR}/../strict_lrelease.cmake"
    -D "TEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/strict_lrelease_tests"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/strict_lrelease_tests.cmake"
)
set_tests_properties(strict_lrelease_tests PROPERTIES LABELS "omim-test")

omim_add_test(editor_dialog_tests editor_dialog_tests.cpp ../editor_dialog.cpp)
target_link_libraries(editor_dialog_tests PRIVATE qt_common)
set_target_properties(editor_dialog_tests PROPERTIES AUTOMOC ON)
omim_add_qt_translations(editor_dialog_tests)
set_tests_properties(editor_dialog_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

omim_add_test(create_feature_dialog_tests create_feature_dialog_tests.cpp ../create_feature_dialog.cpp)
target_link_libraries(create_feature_dialog_tests PRIVATE qt_common)
set_target_properties(create_feature_dialog_tests PROPERTIES AUTOMOC ON)
omim_add_qt_translations(create_feature_dialog_tests)
set_tests_properties(create_feature_dialog_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
