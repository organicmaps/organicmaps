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

omim_add_test(designer_tests designer_tests.cpp)
target_link_libraries(designer_tests PRIVATE qt_designer map Qt6::Widgets)
set_tests_properties(designer_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
if (TARGET generator_tool)
  add_dependencies(designer_tests generator_tool)
  target_compile_definitions(designer_tests PRIVATE TEST_DESIGNER_RECALCULATION)
endif()

add_test(NAME designer_index_script_tests
  COMMAND ${Python3_EXECUTABLE} ${OMIM_ROOT}/tools/python/test_recalculate_geom_index.py
)
set_tests_properties(designer_index_script_tests PROPERTIES LABELS "omim-test")

add_test(NAME designer_style_generation_tests
  COMMAND ${Python3_EXECUTABLE} ${OMIM_ROOT}/tools/python/test_designer_style_generation.py
)
set_tests_properties(designer_style_generation_tests PROPERTIES LABELS "omim-test")

if (PLATFORM_MAC OR PLATFORM_LINUX)
  add_test(NAME designer_package_script_tests
    COMMAND ${Python3_EXECUTABLE} ${OMIM_ROOT}/tools/python/test_package_designer.py
  )
  set_tests_properties(designer_package_script_tests PROPERTIES LABELS "omim-test")
endif()
