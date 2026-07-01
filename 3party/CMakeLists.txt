# Fixes CMake deprecation warning:
# Compatibility with CMake < 3.5 will be removed from a future version of CMake.
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)

if (NOT WITH_SYSTEM_PROVIDED_3PARTY)
  # Suppress "Policy CMP0077 is not set: option() honors normal variables"
  # for the freetype and expat options.
  set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

  add_library(Boost_headers INTERFACE)
  add_library(Boost::headers ALIAS Boost_headers)
  target_include_directories(Boost_headers SYSTEM INTERFACE "${OMIM_ROOT}/3party/boost")

  # Configure expat library.
  set(EXPAT_BUILD_TOOLS OFF)
  set(EXPAT_BUILD_EXAMPLES OFF)
  set(EXPAT_BUILD_TESTS OFF)
  set(EXPAT_BUILD_DOCS OFF)
  set(EXPAT_BUILD_PKGCONFIG OFF)
  set(EXPAT_ENABLE_INSTALL OFF)
  set(EXPAT_SHARED_LIBS OFF)
  set(EXPAT_GE OFF)
  set(EXPAT_DTD OFF)
  set(EXPAT_NS ON)
  add_subdirectory(expat/expat)
  # expat 2.8 moved hash-salt entropy into per-file translation units that define
  # _DEFAULT_SOURCE/_GNU_SOURCE before including <stdlib.h>/<unistd.h>. A unity
  # build concatenates them after other expat sources, so the feature macros are
  # set too late and arc4random()/getentropy() become implicitly declared -- a
  # hard error on glibc + clang. expat is tiny, so disable unity for this target.
  set_target_properties(expat PROPERTIES UNITY_BUILD OFF)

  # Add gflags library.
  set(GFLAGS_BUILD_TESTING OFF)
  set(GFLAGS_BUILD_PACKAGING OFF)
  add_subdirectory(gflags)
  target_compile_options(gflags_nothreads_static PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wno-subobject-linkage>)

  # Add pugixml library.
  add_subdirectory(pugixml)


  add_subdirectory(freetype)
  add_subdirectory(icu)
  add_subdirectory(harfbuzz)

  add_library(utf8cpp INTERFACE)
  add_library(utf8cpp::utf8cpp ALIAS utf8cpp)
  target_include_directories(utf8cpp SYSTEM INTERFACE "${OMIM_ROOT}/3party/utfcpp/source")

  # Build zlib-ng in zlib-compatible mode as a drop-in replacement for the
  # system zlib. It provides the classic zlib API (z_stream/deflate/inflate)
  # used by libs/coding/zlib.* and minizip, plus SIMD-accelerated (de)compression
  # with runtime CPU dispatch (compile-time NEON baseline on armeabi-v7a).
  set(ZLIB_COMPAT ON)                # Provide the classic <zlib.h> API.
  set(WITH_NATIVE_INSTRUCTIONS OFF)  # Portable binaries: no -march=native (keep runtime dispatch).
  set(SKIP_INSTALL_ALL ON)           # Not an installed component.
  # armeabi-v7a: NEON is optional in the ARMv7 spec but present on every Android
  # device we target, so bake it in as a compile-time baseline (no getauxval
  # dispatch). Paired with the ARM_NOCHECK_NEON define after add_subdirectory.
  if (ANDROID_ABI STREQUAL "armeabi-v7a")
    set(WITH_RUNTIME_CPU_DETECTION OFF)
  endif()
  # Force a single static library and skip zlib-ng's own tests, regardless of
  # the parent BUILD_SHARED_LIBS/BUILD_TESTING values.
  set(_om_saved_shared "${BUILD_SHARED_LIBS}")
  set(_om_saved_testing "${BUILD_TESTING}")
  set(BUILD_SHARED_LIBS OFF)
  set(BUILD_TESTING OFF)
  add_subdirectory(zlib-ng)
  set(BUILD_SHARED_LIBS "${_om_saved_shared}")
  set(BUILD_TESTING "${_om_saved_testing}")
  # zlib-ng's template-based sources redefine macros per translation unit, so
  # they cannot be concatenated by OM's global unity build.
  set_target_properties(zlib-ng PROPERTIES UNITY_BUILD OFF)
  # With runtime detection off, zlib-ng's compile-time functable only picks NEON
  # when __ARM_NEON is set in every TU; ARM_NOCHECK_NEON tells it NEON is
  # guaranteed (the NEON sources are still compiled with -mfpu=neon themselves).
  if (ANDROID_ABI STREQUAL "armeabi-v7a")
    target_compile_definitions(zlib-ng PRIVATE ARM_NOCHECK_NEON=1)
  endif()
  # Match the target that find_package(ZLIB) would otherwise provide, so that
  # dependents (coding, minizip) can link ZLIB::ZLIB unchanged.
  if (NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB ALIAS zlib-ng)
  endif()
endif()

add_subdirectory(agg)
add_subdirectory(BLAKE3/c)
add_subdirectory(bsdiff-courgette)
add_subdirectory(glaze)
add_subdirectory(minizip)
add_subdirectory(open-location-code)
add_subdirectory(opening_hours)
add_subdirectory(stb_image)
add_subdirectory(succinct)

add_subdirectory(vulkan_wrapper)

if (PLATFORM_DESKTOP)
  add_subdirectory(libtess2)

  set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
  set(GLFW_INSTALL OFF CACHE BOOL "")
  set(GLFW_VULKAN_STATIC OFF CACHE BOOL "")
  set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "")
  # Disable ARC for glfw and re-enable after it because it's globally set in the root CMakeLists.txt
  set(CMAKE_OBJC_FLAGS "")
  add_subdirectory(glfw)
  set_target_properties(glfw PROPERTIES UNITY_BUILD OFF)
  set_target_properties(glfw PROPERTIES XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC NO)
  set(CMAKE_OBJC_FLAGS -fobjc-arc)

  add_subdirectory(imgui)
endif()
