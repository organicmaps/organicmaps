project(dev_sandbox)

set(SRC
  main.cpp
  imgui_renderer.cpp
  imgui_renderer.hpp
)

if (PLATFORM_MAC)
  # Metal language support. dev_sandbox is its only CMake consumer: iOS compiles
  # the same libs/shaders/Metal/*.metal via xcode/omim.xcworkspace.
  list(APPEND CMAKE_MODULE_PATH ${OMIM_ROOT}/3party/CMake-MetalShaderSupport/cmake)
  include(CheckLanguage)
  include(MetalShaderSupport)

  # The Metal compiler lives in an on-demand cryptex mount whose path contains a
  # suffix that changes on every remount, so a cached path goes dead. Drop it and
  # let check_language() retry, as it skips detection when the variable is set,
  # together with the compiler info file, which enable_language() would otherwise
  # load in place of the freshly detected path.
  if (NOT EXISTS "${CMAKE_Metal_COMPILER}")
    unset(CMAKE_Metal_COMPILER CACHE)
    file(REMOVE "${CMAKE_PLATFORM_INFO_DIR}/CMakeMetalCompiler.cmake")
  endif()
  check_language(Metal)

  # While that mount is down xcrun answers with a stub in XcodeDefault.xctoolchain
  # that does exist but fails to run. check_language()'s try_compile rejects it,
  # except with the Xcode generator, where CMakeTestMetalCompiler assumes a
  # working compiler and skips the test, so probe it directly. Clear the cache as
  # well, otherwise the stub would be picked up again once the mount is back.
  if (CMAKE_Metal_COMPILER)
    execute_process(COMMAND "${CMAKE_Metal_COMPILER}" --version
                    RESULT_VARIABLE METAL_COMPILER_RUNS OUTPUT_QUIET ERROR_QUIET)
    if (NOT METAL_COMPILER_RUNS EQUAL 0)
      unset(CMAKE_Metal_COMPILER CACHE)
      unset(CMAKE_Metal_COMPILER)
      file(REMOVE "${CMAKE_PLATFORM_INFO_DIR}/CMakeMetalCompiler.cmake")
    endif()
  endif()

  if (NOT CMAKE_Metal_COMPILER)
    # The shaders target below would still be generated, but with unexpanded rule
    # placeholders that fail the build with a cryptic message.
    message(WARNING "Skipping dev_sandbox: no Metal toolchain."
                    " Install it with: xcodebuild -downloadComponent MetalToolchain")
    return()
  endif()
  enable_language(Metal)

  append(SRC
    main.mm
    ../iphone/Maps/Core/MapRendering/MetalContextFactory.h
    ../iphone/Maps/Core/MapRendering/MetalContextFactory.mm
  )

  file(GLOB_RECURSE SHADER_SOURCES_FILES ${OMIM_ROOT}/libs/shaders/Metal/*.metal)
  add_metal_shader_library(shaders_metal
    ${SHADER_SOURCES_FILES}
  )
endif()

if (${PLATFORM_LINUX})
  append(SRC
    main_linux.cpp
  )
endif()

if (PLATFORM_WIN)
  append(SRC
    main_windows.cpp
  )
endif()

omim_add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SRC})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    drape
    generator  # For borders::LoadBorders
    map
    gflags::gflags
    glfw
    imgui
    vulkan_wrapper
)

# Installing Vulkan SDK is optional, however without it Vulkan dynamic libraries
# should be discovered via system paths and validation layers may not be available
find_package(Vulkan QUIET)
if (Vulkan_FOUND)
  message(STATUS "Vulkan found")
  target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan)
endif()

target_compile_definitions(${PROJECT_NAME} PUBLIC GL_SILENCE_DEPRECATION)
if (PLATFORM_MAC)
  target_embed_metal_shader_libraries(${PROJECT_NAME} shaders_metal)
  set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
  # Xcode's multi-config generator puts the .app under $(CONFIGURATION)/ by default,
  # while copy_resources() writes directly to ${CMAKE_BINARY_DIR}/${BUNDLE_NAME}.app.
  # Force Xcode to drop the per-config subdir so resources end up in the running bundle.
  if (CMAKE_GENERATOR STREQUAL "Xcode")
    set_target_properties(${PROJECT_NAME} PROPERTIES
      XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR "${CMAKE_BINARY_DIR}")
  endif()
endif()

target_compile_definitions(${PROJECT_NAME}
  PRIVATE
    $<$<BOOL:${PLATFORM_LINUX}>:VK_USE_PLATFORM_XLIB_KHR>
    $<$<BOOL:${PLATFORM_WIN}>:VK_USE_PLATFORM_WIN32_KHR>
    $<$<BOOL:${PLATFORM_ANDROID}>:VK_USE_PLATFORM_ANDROID_KHR>
)

target_include_directories(${PROJECT_NAME} PUBLIC ${OMIM_ROOT}/3party/glfw/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${OMIM_ROOT}/3party/imgui)

set(BUNDLE_NAME "OMapsDevSandbox")
set(BUNDLE_DISPLAY_NAME "Organic Maps: Developer Sandbox")

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${BUNDLE_NAME})

set(BUNDLE_EXECUTABLE ${BUNDLE_NAME})

set(BUNDLE_FOLDER ${CMAKE_BINARY_DIR}/${BUNDLE_NAME}.app)
set(RESOURCES_FOLDER ${BUNDLE_FOLDER}/Contents/Resources)
set(DATA_DIR ${OMIM_ROOT}/data)

function(copy_resources)
  execute_process(COMMAND mkdir -p ${RESOURCES_FOLDER})
  foreach(file ${ARGN})
    execute_process(
      COMMAND cp -r ${DATA_DIR}/${file} ${RESOURCES_FOLDER}
      COMMAND_ERROR_IS_FATAL ANY
    )
  endforeach()
endfunction()

copy_resources(
  fonts
  symbols
  countries-strings
  vulkan_shaders

  categories.txt
  categories_cuisines.txt
  classificator.txt
  colors.txt
  countries.json
  drules_default.bin
  drules_outdoors.bin
  drules_vehicle.bin
  editor.config
  packed_polygons.bin
  patterns.txt
  transit_colors.txt
  types.txt
  World.mwm
  WorldCoasts.mwm
)

if (NOT WITH_SYSTEM_PROVIDED_3PARTY)
  copy_resources(icudt78l.dat)
endif()
