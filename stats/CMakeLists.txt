project(stats_client)

include_directories(${OMIM_ROOT} ${OMIM_ROOT}/3party/Alohalytics/src)

set(
  SRC
  ${OMIM_ROOT}/3party/Alohalytics/src/cpp/alohalytics.cc
  ${OMIM_ROOT}/3party/Alohalytics/src/alohalytics.h
  ${OMIM_ROOT}/3party/Alohalytics/src/event_base.h
  ${OMIM_ROOT}/3party/Alohalytics/src/file_manager.h
  ${OMIM_ROOT}/3party/Alohalytics/src/http_client.h
  ${OMIM_ROOT}/3party/Alohalytics/src/logger.h
)

if (${PLATFORM_WIN})
  append(
    SRC
    ${OMIM_ROOT}/3party/Alohalytics/src/windows/file_manager_windows_impl.cc
  )
else()
  append(
    SRC
    ${OMIM_ROOT}/3party/Alohalytics/src/posix/file_manager_posix_impl.cc
  )
endif()

if (${PLATFORM_IPHONE} OR ${PLATFORM_MAC})
  append(
    SRC
    ${OMIM_ROOT}/3party/Alohalytics/src/alohalytics_objc.h
    ${OMIM_ROOT}/3party/Alohalytics/src/apple/http_client_apple.mm
    ${OMIM_ROOT}/3party/Alohalytics/src/apple/alohalytics_objc.mm
  )
elseif(${PLATFORM_LINUX} OR ${PLATFORM_WIN})
  append(
    SRC
    ${OMIM_ROOT}/3party/Alohalytics/src/posix/http_client_curl.cc
  )
elseif(${PLATFORM_ANDROID})
  append(
    SRC
    ${OMIM_ROOT}/3party/Alohalytics/src/android/jni/jni_alohalytics.cc
  )

endif()

omim_add_library(${PROJECT_NAME} ${SRC})

if (${PLATFORM_IPHONE} OR ${PLATFORM_MAC})
  target_compile_options(${PROJECT_NAME} PUBLIC "-fobjc-arc")
endif()
