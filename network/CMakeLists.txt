project(network)

set(PUBLIC_SOURCES
  downloader/utils.cpp
  downloader/utils.hpp
  http/background_uploader.hpp
  http/client.cpp
  http/client.hpp
  http/payload.hpp
  http/request.cpp
  http/request.hpp
  http/thread.hpp
  http/thread_callback.hpp
  http/uploader.hpp
  chunks_download_strategy.cpp
  chunks_download_strategy.hpp
  download_status.hpp
  non_http_error_code.hpp
  progress.hpp
  servers_list.cpp
  servers_list.hpp
  socket.hpp
)

omim_add_library(${PROJECT_NAME} ${PUBLIC_SOURCES})

target_sources(${PROJECT_NAME} PRIVATE
  internal/http/file_request.hpp
  internal/http/memory_request.hpp
)

if (PLATFORM_MAC)
  target_sources(${PROJECT_NAME} PRIVATE
    internal/native/apple/http/background_uploader.mm
    internal/native/apple/http/client.mm
    internal/native/apple/http/session_manager.h
    internal/native/apple/http/session_manager.mm
    internal/native/apple/http/socket.mm
    internal/native/apple/http/thread.h
    internal/native/apple/http/thread.mm
    internal/native/apple/http/uploader.mm
    internal/native/apple/background_downloader.h
    internal/native/apple/background_downloader.mm
    internal/native/apple/network_policy.h
    internal/native/apple/network_policy.mm
  )
endif ()

if (PLATFORM_WIN OR PLATFORM_LINUX)
  target_sources(${PROJECT_NAME} PRIVATE
    internal/http/dummy_background_uploader.cpp
    internal/http/dummy_uploader.cpp
    internal/native/desktop/http/client.cpp
    internal/native/desktop/http/thread.cpp
    internal/native/desktop/http/thread.hpp
    internal/dummy_socket.cpp
  )

  set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC ON)
endif ()

target_link_libraries(${PROJECT_NAME}
  PUBLIC base
  PRIVATE coding
  PRIVATE platform
  PRIVATE $<$<BOOL:${PLATFORM_LINUX}>:Qt6::Network>
  PRIVATE $<$<BOOL:${PLATFORM_WIN}>:Qt6::Network>
  PRIVATE $<$<BOOL:${PLATFORM_MAC}>:
  -framework\ Foundation
  -framework\ CFNetwork
  -framework\ Security  # SecPKCS12Import
  >
)

omim_add_test_subdirectory(network_tests_support)
omim_add_test_subdirectory(network_tests)
