# TODO: Check if enabling it provides benefits.
set(FT_DISABLE_HARFBUZZ ON)

# Don't link an external zlib: FreeType would otherwise find_package(ZLIB) and
# pull in the *system* zlib alongside our zlib-ng (two zlib implementations in
# one binary, and a system dependency that is absent on Windows). FreeType falls
# back to its own bundled zlib for the rare gzip-compressed font tables.
set(FT_DISABLE_ZLIB ON)

add_subdirectory(freetype)

# Fix warning with ONE_PIXEL macro clash.
if(NOT MSVC)
 target_compile_options(freetype PRIVATE -Wno-macro-redefined)
endif()

# Use ft2build.h from the current directory instead of the default.
target_include_directories(freetype
  BEFORE PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include/freetype2>
)

add_library(Freetype::Freetype ALIAS freetype)

