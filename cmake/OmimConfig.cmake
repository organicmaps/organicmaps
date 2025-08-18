# Flags for all
set(OMIM_WARNING_FLAGS
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wno-unused-parameter>  # We have a lot of functions with unused parameters
)
set(3PARTY_INCLUDE_DIRS "${OMIM_ROOT}/3party/boost")
set(OMIM_DATA_DIR "${OMIM_ROOT}/data")
set(OMIM_USER_RESOURCES_DIR "${OMIM_ROOT}/data")

# GCC 10.0 is required to support <charconv> header inclusion in base/string_utils.hpp
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.0)
  message(FATAL_ERROR "Minimum supported g++ version is 10.0, yours is ${CMAKE_CXX_COMPILER_VERSION}")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(PCH_EXTENSION "pch")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  set(PCH_EXTENSION "gch")
endif()

if (NJOBS)
  message(STATUS "Number of parallel processes: ${NJOBS}")
  set(CMAKE_JOB_POOLS custom=${NJOBS})
  set(CMAKE_JOB_POOL_COMPILE custom)
  set(CMAKE_JOB_POOL_LINK custom)
  set(CMAKE_JOB_POOL_PRECOMPILE_HEADER custom)
endif()

if (USE_ASAN AND USE_TSAN)
  message(FATAL_ERROR "Can't use asan and tsan sanitizers together")
elseif (USE_ASAN)
  message(STATUS "Address Sanitizer is enabled")
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
elseif (USE_TSAN)
  message(STATUS "Thread Sanitizer is enabled")
  add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
endif()

if (USE_LIBFUZZER)
  message(STATUS "LibFuzzer is enabled")
  add_compile_options(-fsanitize=fuzzer)
endif()

if (USE_PPROF)
  message(STATUS "Google Profiler is enabled")
  add_definitions(-DUSE_PPROF)
endif()

if (USE_HEAPPROF)
  message(STATUS "Heap Profiler is enabled")
endif()

if (ENABLE_VULKAN_DIAGNOSTICS)
  message(WARNING "Vulkan diagnostics are enabled. Be aware of performance impact!")
  add_definitions(-DENABLE_VULKAN_DIAGNOSTICS)
endif()

if (ENABLE_TRACE)
  message(STATUS "Tracing is enabled")
  add_definitions(-DENABLE_TRACE)
endif()

if (BUILD_DESIGNER)
  message(STATUS "Designer tool building is enabled")
  add_definitions(-DBUILD_DESIGNER)
endif()

if (BUILD_STANDALONE)
  message(STATUS "Standalone building is enabled")
endif()
