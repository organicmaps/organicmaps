# TODO: Check if enabling it provides benefits.
set(FT_DISABLE_HARFBUZZ ON)

# Don't link an external zlib: FreeType would otherwise find_package(ZLIB) and
# pull in the *system* zlib alongside our zlib-ng (two zlib implementations in
# one binary, and a system dependency that is absent on Windows). FreeType falls
# back to its own bundled zlib for the rare gzip-compressed font tables.
set(FT_DISABLE_ZLIB ON)

add_subdirectory(freetype)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
  # sdf.c conflicts with smooth.c's ONE_PIXEL; autofit.c triggers GCC's unity-only bounds warning.
  set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/freetype/src/sdf/sdf.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/freetype/src/autofit/autofit.c"
    TARGET_DIRECTORY freetype
    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
  )
else()
  target_compile_options(freetype PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-macro-redefined>)
endif()

# Use ft2build.h from the current directory instead of the default.
target_include_directories(freetype
  BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/freetype2>
)

add_library(Freetype::Freetype ALIAS freetype)

