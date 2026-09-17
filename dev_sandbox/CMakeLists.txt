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

  # The Metal compiler lives in an on-demand cryptex mount whose path suffix changes on
  # every remount, so a cached path goes dead. check_language() skips detection while the
  # variable is set (NOTFOUND included), and enable_language() would reuse the dead path
  # recorded in the info file, which upstream re-resolves only at an unchanged version.
  if (NOT EXISTS "${CMAKE_Metal_COMPILER}")
    unset(CMAKE_Metal_COMPILER CACHE)
    file(REMOVE "${CMAKE_PLATFORM_INFO_DIR}/CMakeMetalCompiler.cmake")
  endif()
  check_language(Metal)

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
    $<$<BOOL:${PLATFORM_LINUX}>:VK_USE_PLATFORM_WAYLAND_KHR>
    $<$<BOOL:${PLATFORM_WIN}>:VK_USE_PLATFORM_WIN32_KHR>
    $<$<BOOL:${PLATFORM_ANDROID}>:VK_USE_PLATFORM_ANDROID_KHR>
)

if (PLATFORM_LINUX)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(EGL REQUIRED IMPORTED_TARGET egl)
  pkg_check_modules(WAYLAND_EGL REQUIRED IMPORTED_TARGET wayland-egl)
  target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::EGL PkgConfig::WAYLAND_EGL)
endif()

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
