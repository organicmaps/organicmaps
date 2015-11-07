# Build file for MAPS.ME project
#
# Possible options:
#   gtool: build only generator_tool
#   map_designer: enable designer-related flags
#   no-tests: do not build tests for desktop
#   drape: include drape libraries
#
# There are no supported options in CONFIG for mobile platforms.
# Please use XCode or gradle/Android Studio.


lessThan(QT_MAJOR_VERSION, 5) {
  error("You need Qt 5 to build this project. You have Qt $$QT_VERSION")
}

cache()

TEMPLATE = subdirs

HEADERS += defines.hpp

# TODO(AlexZ): Why is it here? Drape should build on win32 too.
win32:CONFIG(drape) {
  CONFIG -= drape
}

!iphone*:!tizen*:!android* {
  CONFIG *= desktop
}

SUBDIRS = 3party base coding geometry indexer routing

!CONFIG(osrm) {
  SUBDIRS *= platform stats storage

  # Integration tests dependencies for gtool.
  # TODO(AlexZ): Avoid duplication for routing_integration_tests.
  CONFIG(gtool):!CONFIG(no-tests) {
    SUBDIRS *= search map

    routing_integration_tests.subdir = routing/routing_integration_tests
    routing_integration_tests.depends = $$SUBDIRS
    SUBDIRS *= routing_integration_tests
  }

  CONFIG(desktop) {
    SUBDIRS *= generator

    generator_tool.subdir = generator/generator_tool
    generator_tool.depends = $$SUBDIRS
    SUBDIRS *= generator_tool
  }
}

!CONFIG(gtool):!CONFIG(osrm) {
  SUBDIRS *= anim graphics gui render search map

  CONFIG(map_designer):CONFIG(desktop) {
    SUBDIRS *= skin_generator
  }

  CONFIG(drape) {
    SUBDIRS *= drape drape_frontend
    CONFIG(desktop) {
      drape_head.depends = $$SUBDIRS
      SUBDIRS *= drape_head
    }
  }

  CONFIG(desktop) {
    benchmark_tool.subdir = map/benchmark_tool
    benchmark_tool.depends = 3party base coding geometry platform indexer map
    SUBDIRS *= benchmark_tool

    qt.depends = $$SUBDIRS
    SUBDIRS *= qt
  }

  CONFIG(desktop):!CONFIG(no-tests) {
    # Additional desktop-only, tests-only libraries.
    platform_tests_support.subdir = platform/platform_tests_support
    SUBDIRS *= platform_tests_support

    # Tests binaries.
    base_tests.subdir = base/base_tests
    base_tests.depends = base
    SUBDIRS *= base_tests

    coding_tests.subdir = coding/coding_tests
    coding_tests.depends = 3party base
    SUBDIRS *= coding_tests

    geometry_tests.subdir = geometry/geometry_tests
    geometry_tests.depends = 3party base geometry indexer
    SUBDIRS *= geometry_tests

    indexer_tests.subdir = indexer/indexer_tests
    indexer_tests.depends = 3party base coding geometry indexer
    SUBDIRS *= indexer_tests

    platform_tests.subdir = platform/platform_tests
    platform_tests.depends = 3party base coding platform platform_tests_support
    SUBDIRS *= platform_tests

    downloader_tests.subdir = platform/downloader_tests
    downloader_tests.depends = 3party base coding platform platform_tests_support
    SUBDIRS *= downloader_tests

    storage_tests.subdir = storage/storage_tests
    storage_tests.depends = 3party base coding geometry platform storage indexer stats
    SUBDIRS *= storage_tests

    search_tests.subdir = search/search_tests
    search_tests.depends = 3party base coding geometry platform indexer search
    SUBDIRS *= search_tests

    search_integration_tests.subdir = search/search_integration_tests
    search_integration_tests.depends = 3party base coding geometry platform indexer search
    SUBDIRS *= search_integration_tests

    MapDepLibs = 3party base coding geometry platform storage indexer search map
    map_tests.subdir = map/map_tests
    map_tests.depends = $$MapDepLibs
    SUBDIRS *= map_tests

    mwm_tests.subdir = map/mwm_tests
    mwm_tests.depends = $$MapDepLibs
    SUBDIRS *= mwm_tests

    style_tests.subdir = map/style_tests
    style_tests.depends = $$MapDepLibs
    SUBDIRS *= style_tests

    routing_tests.subdir = routing/routing_tests
    routing_tests.depends = $$MapDepLibs routing
    SUBDIRS *= routing_tests

    routing_integration_tests.subdir = routing/routing_integration_tests
    routing_integration_tests.depends = $$MapDepLibs routing
    SUBDIRS *= routing_integration_tests

    # TODO(AlexZ): Move pedestrian tests into routing dir.
    pedestrian_routing_tests.depends = $$MapDepLibs routing
    SUBDIRS *= pedestrian_routing_tests

    generator_tests.subdir = generator/generator_tests
    generator_tests.depends = $$MapDepLibs routing generator
    SUBDIRS *= generator_tests

    # TODO(AlexZ): Do we really need them?
    #SUBDIRS += render/render_tests
    #SUBDIRS += graphics/graphics_tests
    #SUBDIRS += gui/gui_tests

    CONFIG(drape) {
      SUBDIRS *= qt_tstfrm

      drape_tests.subdir = drape/drape_tests
      drape_tests.depends = 3party base coding platform qt_tstfrm
      SUBDIRS *= drape_tests

      drape_frontend_tests.subdir = drape_frontend/drape_frontend_tests
      drape_frontend_tests.depends = 3party base coding platform drape drape_frontend
      SUBDIRS *= drape_frontend_tests
    }
  } # !no-tests
} # !gtool
