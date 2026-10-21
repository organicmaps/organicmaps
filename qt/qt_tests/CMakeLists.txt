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

omim_add_test(bookmark_export_tests NO_PLATFORM_INIT
  bookmark_export_tests.cpp
  ../bookmark_export.cpp
)
target_link_libraries(bookmark_export_tests PRIVATE Qt6::Core)

omim_add_test(forwarding_quickwidget_tests NO_PLATFORM_INIT
  forwarding_quickwidget_tests.cpp
  ../qml/forwarding_quickwidget.cpp
)
target_link_libraries(forwarding_quickwidget_tests PRIVATE Qt6::QuickWidgets Qt6::Test)
set_tests_properties(forwarding_quickwidget_tests PROPERTIES
  ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software"
)
