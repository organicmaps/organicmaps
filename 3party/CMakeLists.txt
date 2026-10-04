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
endif()
