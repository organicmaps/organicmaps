project(harfbuzz)

set(SOURCES
  harfbuzz/src/harfbuzz.cc
)

add_library(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
  PUBLIC
    harfbuzz/src
)

# Keep these settigns in sync with xcode/harfbuzz project.
target_compile_options(${PROJECT_NAME}
  PRIVATE
    -fno-rtti
    -fno-exceptions
    -fno-threadsafe-statics
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-Wno-format-pedantic>
)

target_compile_definitions(${PROJECT_NAME}
  PRIVATE
    HAVE_FREETYPE=1
    # TODO: Enable later if necessary, and sync with xcode/harfbuzz project.
    #HAVE_ICU
    #$<$<BOOL:${APPLE}>:HAVE_CORETEXT>
    HAVE_ATEXIT
    HAVE_GETPAGESIZE
    HAVE_MMAP
    HAVE_MPROTECT
    HAVE_PTHREAD
    HAVE_SYSCONF
    HAVE_SYS_MMAN_H
    HAVE_UNISTD_H
)

target_link_libraries(${PROJECT_NAME} Freetype::Freetype)

add_library(harfbuzz::harfbuzz ALIAS harfbuzz)
