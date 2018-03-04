project(platform)

include_directories(${OMIM_ROOT}/3party/jansson/src ${OMIM_ROOT}/platform)

set(CMAKE_AUTOMOC ON)

set(
  SRC
  chunks_download_strategy.cpp
  chunks_download_strategy.hpp
  constants.hpp
  country_defines.cpp
  country_defines.hpp
  country_file.cpp
  country_file.hpp
  file_logging.cpp
  file_logging.hpp
  get_text_by_id.cpp
  get_text_by_id.hpp
  gui_thread.hpp
  http_client.cpp
  http_client.hpp
  http_request.cpp
  http_request.hpp
  http_thread_callback.hpp
  http_uploader.hpp
  local_country_file.cpp
  local_country_file.hpp
  local_country_file_utils.cpp
  local_country_file_utils.hpp
  location.hpp
  marketing_service.cpp
  marketing_service.hpp
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
  servers_list.cpp
  servers_list.hpp
  settings.cpp
  settings.hpp
  socket.hpp
  string_storage_base.cpp
  string_storage_base.hpp
)

if(${PLATFORM_IPHONE})
  append(
    SRC
    gui_thread_apple.mm
    http_thread_apple.h
    http_thread_apple.mm
    http_client_apple.mm
    http_uploader_apple.mm
    marketing_service_ios.mm
    network_policy_ios.h
    network_policy_ios.mm
    platform_ios.mm
    platform_unix_impl.cpp
    platform_unix_impl.hpp
    secure_storage_ios.mm
    socket_apple.mm
  )
elseif(${PLATFORM_ANDROID})
  append(
    SRC
    platform_android.cpp
    platform_unix_impl.cpp
    platform_unix_impl.hpp
  )
else() # neither iPhone nor Android
  append(
    SRC
    location_service.cpp
    location_service.hpp
    marketing_service_dummy.cpp
    network_policy_dummy.cpp
    platform_qt.cpp
    wifi_info.hpp
    wifi_location_service.cpp
  )

  if (${PLATFORM_WIN})
    append(
      SRC
      http_client_curl.cpp
      http_thread_qt.cpp
      http_thread_qt.hpp
      http_uploader_dummy.cpp
      marketing_service_dummy.cpp
      platform_win.cpp
      secure_storage_dummy.cpp
      wifi_info_windows.cpp
    )
  elseif(${PLATFORM_MAC})
    append(
      SRC
      apple_location_service.mm
      gui_thread_apple.mm
      http_client_apple.mm
      http_thread_apple.h
      http_thread_apple.mm
      http_uploader_apple.mm
      http_uploader_apple.swift
      marketing_service_dummy.cpp
      platform_mac.mm
      platform_unix_impl.cpp
      platform_unix_impl.hpp
      secure_storage_qt.cpp
      socket_apple.mm
    )
  elseif(${PLATFORM_LINUX})
    append(
      SRC
      gui_thread_linux.cpp
      http_client_curl.cpp
      http_thread_qt.cpp
      http_thread_qt.hpp
      http_uploader_dummy.cpp
      marketing_service_dummy.cpp
      platform_linux.cpp
      platform_unix_impl.cpp
      platform_unix_impl.hpp
      secure_storage_qt.cpp
    )
  endif()
endif()

add_subdirectory(platform_tests_support)

add_library(${PROJECT_NAME} ${SRC})

# 2 next lines are added for future support of swift code compilation.
set(SWIFT_BRIDGING_HEADER platform-Swift.h)
set_property(TARGET ${PROJECT_NAME} PROPERTY XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${SWIFT_BRIDGING_HEADER}")

if (APPLE)
# "-DIGNORE_SWIFT=1" key is added as the temporary solution until support of swift code compilation.
  target_compile_options(${PROJECT_NAME} PUBLIC "-fobjc-arc" "-DIGNORE_SWIFT=1")
endif()

omim_add_test_subdirectory(platform_tests)
