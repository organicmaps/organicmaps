project(platform)

set(SRC
  ../private.h  # To properly detect its changes with CMake.
  battery_tracker.cpp
  battery_tracker.hpp
  chunks_download_strategy.cpp
  chunks_download_strategy.hpp
  constants.hpp
  country_defines.cpp
  country_defines.hpp
  country_file.cpp
  country_file.hpp
  downloader_defines.hpp
  downloader_utils.cpp
  downloader_utils.hpp
  get_text_by_id.cpp
  get_text_by_id.hpp
  gui_thread.hpp
  http_client.cpp
  http_client.hpp
  http_payload.cpp
  http_payload.hpp
  http_request.cpp
  http_request.hpp
  http_thread_callback.hpp
  http_uploader.hpp
  http_uploader_background.hpp
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
  remote_file.cpp
  remote_file.hpp
  secure_storage.hpp
  servers_list.cpp
  servers_list.hpp
  settings.cpp
  settings.hpp
  socket.hpp
  string_storage_base.cpp
  string_storage_base.hpp
  utm_mgrs_utils.cpp
  utm_mgrs_utils.hpp
)

if(${PLATFORM_IPHONE})
  append(SRC
    background_downloader_ios.h
    background_downloader_ios.mm
    gui_thread_apple.mm
    http_thread_apple.h
    http_thread_apple.mm
    http_client_apple.mm
    http_uploader_apple.mm
    http_user_agent_ios.mm
    localization.mm
    locale.mm
    network_policy_ios.h
    network_policy_ios.mm
    platform_ios.mm
    platform_unix_impl.cpp
    platform_unix_impl.hpp
    secure_storage_ios.mm
    socket_apple.mm
  )
elseif(${PLATFORM_ANDROID})
  append(SRC
    platform_android.cpp
    platform_unix_impl.cpp
    platform_unix_impl.hpp
  )
else() # neither iPhone nor Android
  # Find bash first, on Windows it can be either in Git or in WSL
  find_program(BASH bash)
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
    network_policy_dummy.cpp
    platform_qt.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/platform_qt_version.cpp"
  )

  if (${PLATFORM_WIN})
    append(SRC
      http_client_curl.cpp
      http_thread_qt.cpp
      http_thread_qt.hpp
      http_uploader_background_dummy.cpp
      http_uploader_dummy.cpp
      platform_win.cpp
      secure_storage_dummy.cpp
    )
  elseif(${PLATFORM_MAC})
    append(SRC
      apple_location_service.mm
      gui_thread_apple.mm
      http_client_apple.mm
      http_thread_apple.h
      http_thread_apple.mm
      http_uploader_apple.mm
      http_uploader_background_dummy.cpp
      locale.mm
      platform_mac.mm
      platform_unix_impl.cpp
      platform_unix_impl.hpp
      secure_storage_qt.cpp
      socket_apple.mm
      http_session_manager.mm
    )
  elseif(${PLATFORM_LINUX})
    append(SRC
      gui_thread_linux.cpp
      http_client_curl.cpp
      http_thread_qt.cpp
      http_thread_qt.hpp
      http_uploader_dummy.cpp
      http_uploader_background_dummy.cpp
      locale_linux.cpp
      platform_linux.cpp
      platform_unix_impl.cpp
      platform_unix_impl.hpp
      secure_storage_qt.cpp
    )
  endif()
endif()

omim_add_library(${PROJECT_NAME} ${SRC})

if (APPLE)
  target_compile_options(${PROJECT_NAME} PRIVATE "-fobjc-arc")
endif()

if (PLATFORM_LINUX)
  set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)
endif()

target_link_libraries(${PROJECT_NAME}
  geometry  # mercator::YToLat
  coding
  $<$<BOOL:${PLATFORM_DESKTOP}>:Qt5::Core>
  $<$<BOOL:${PLATFORM_LINUX}>:Qt5::Network>
  $<$<BOOL:${PLATFORM_MAC}>:-framework\ Foundation -framework\ SystemConfiguration -framework\ CoreLocation -framework\ CFNetwork>
)

omim_add_test_subdirectory(platform_tests_support)
omim_add_test_subdirectory(platform_tests)

# strings::UniChar clashes with Apple's definition.
set_property(SOURCE preferred_languages.cpp PROPERTY SKIP_UNITY_BUILD_INCLUSION ON)
