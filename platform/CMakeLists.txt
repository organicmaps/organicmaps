project(platform)

set(SRC
  ../private.h  # To properly detect its changes with CMake.
  battery_tracker.cpp
  battery_tracker.hpp
  constants.hpp
  country_defines.cpp
  country_defines.hpp
  country_file.cpp
  country_file.hpp
  distance.cpp
  distance.hpp
  duration.cpp
  duration.hpp
  get_text_by_id.cpp
  get_text_by_id.hpp
  gui_thread.hpp
  local_country_file.cpp
  local_country_file.hpp
  local_country_file_utils.cpp
  local_country_file_utils.hpp
  locale.hpp
  localization.cpp
  localization.hpp
  location.hpp
  measurement_utils.cpp
  measurement_utils.hpp
  mwm_traits.cpp
  mwm_traits.hpp
  mwm_version.cpp
  mwm_version.hpp
  platform.cpp
  platform.hpp
  preferred_languages.cpp
  preferred_languages.hpp
  secure_storage.hpp
  settings.cpp
  settings.hpp
  trace.hpp
  string_storage_base.cpp
  string_storage_base.hpp
  utm_mgrs_utils.cpp
  utm_mgrs_utils.hpp
)

if (NOT SKIP_QT_GUI AND PLATFORM_DESKTOP AND PLATFORM_LINUX)
  message("Building with Qt Positioning")
  find_package(Qt6 REQUIRED COMPONENTS Positioning)
  set(QT_POSITIONING true)
  append(SRC
    qt_location_service.hpp
    qt_location_service.cpp
  )
else()
  message("Building without Qt Positioning")
  set(QT_POSITIONING false)
endif()

if (PLATFORM_IPHONE)
  append(SRC
    gui_thread_apple.mm
    localization.mm
    locale.mm
    platform_ios.mm
    platform_unix_impl.cpp
    platform_unix_impl.hpp
    secure_storage_ios.mm
  )
elseif(${PLATFORM_ANDROID})
  append(SRC
    platform_android.cpp
    platform_unix_impl.cpp
    platform_unix_impl.hpp
    trace_android.cpp
  )
else() # neither iPhone nor Android
  # Find bash first, on Windows it can be either in Git or in WSL
  find_program(BASH bash REQUIRED)
  # Generate version header file.
  execute_process(COMMAND "${BASH}" tools/unix/version.sh qt_version
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE OM_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY
  )
  execute_process(COMMAND "${BASH}" tools/unix/version.sh qt_int_version
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE OM_INT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY
  )
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/platform_qt_version.cpp.in"
                 "${CMAKE_CURRENT_BINARY_DIR}/platform_qt_version.cpp"
                  @ONLY)

  append(SRC
    localization_dummy.cpp
    location_service.cpp
    location_service.hpp
    platform_qt.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/platform_qt_version.cpp"
  )

  if (${PLATFORM_WIN})
    append(SRC
      gui_thread_qt.cpp
      locale_std.cpp
      platform_win.cpp
      secure_storage_dummy.cpp
    )
  elseif(${PLATFORM_MAC})
    append(SRC
      apple_location_service.mm
      gui_thread_apple.mm
      locale.mm
      platform_mac.mm
      platform_unix_impl.cpp
      platform_unix_impl.hpp
      secure_storage_qt.cpp
    )
  elseif(${PLATFORM_LINUX})
    append(SRC
      gui_thread_qt.cpp
      locale_std.cpp
      platform_linux.cpp
      platform_unix_impl.cpp
      platform_unix_impl.hpp
      secure_storage_qt.cpp
    )
  endif()
endif()

omim_add_library(${PROJECT_NAME} ${SRC})
if (QT_POSITIONING)
  target_compile_definitions(${PROJECT_NAME} PRIVATE "QT_LOCATION_SERVICE")
endif()

if (APPLE)
  target_compile_options(${PROJECT_NAME} PRIVATE -fobjc-arc -Wno-nullability-completeness)
endif()

if (PLATFORM_LINUX OR PLATFORM_WIN)
  set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)
endif()

target_link_libraries(${PROJECT_NAME}
  geometry  # mercator::YToLat
  coding
  $<$<BOOL:${PLATFORM_DESKTOP}>:Qt6::Core>
  $<$<BOOL:${QT_POSITIONING}>:Qt6::Positioning>
  $<$<BOOL:${PLATFORM_MAC}>:
    -framework\ Foundation
    -framework\ SystemConfiguration
    -framework\ CoreLocation
    >
)

omim_add_test_subdirectory(platform_tests_support)
omim_add_test_subdirectory(platform_tests)

# strings::UniChar clashes with Apple's definition.
set_property(SOURCE preferred_languages.cpp PROPERTY SKIP_UNITY_BUILD_INCLUSION ON)
