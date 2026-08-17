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
set_tests_properties(update_dialog_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
