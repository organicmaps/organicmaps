# Build file for MAPS.ME project
#
# Possible options:
#   gtool: build only generator_tool
#   map_designer: enable designer-related flags
#   no-tests: do not build tests for desktop
#
# There are no supported options in CONFIG for mobile platforms.
# Please use XCode or gradle/Android Studio.


lessThan(QT_MAJOR_VERSION, 5) {
  error("You need Qt 5 to build this project. You have Qt $$QT_VERSION")
}

cache()

TEMPLATE = subdirs

HEADERS += defines.hpp

!iphone*:!tizen*:!android* {
  CONFIG *= desktop
}

SUBDIRS = 3party base coding geometry editor indexer routing search

!CONFIG(osrm) {
  SUBDIRS *= platform stats storage

  # Integration tests dependencies for gtool.
  # TODO(AlexZ): Avoid duplication for routing_integration_tests.
  CONFIG(gtool):!CONFIG(no-tests) {
    SUBDIRS *= map

    routing_integration_tests.subdir = routing/routing_integration_tests
    routing_integration_tests.depends = $$SUBDIRS
    routing_consistency_tests.subdir = routing/routing_consistency_tests
    routing_consistency_tests.depends = $$SUBDIRS
    srtm_source_tests.subdir = generator/srtm_source_tests
    srtm_source_tests.depends = $$SUBDIRS routing
    SUBDIRS *= routing_integration_tests routing_consistency_tests srtm_source_tests
  }

  CONFIG(desktop) {
    SUBDIRS *= generator

    generator_tool.subdir = generator/generator_tool
    generator_tool.depends = $$SUBDIRS
    SUBDIRS *= generator_tool
  }
}

!CONFIG(gtool):!CONFIG(osrm) {
  SUBDIRS *= drape drape_frontend map

  CONFIG(map_designer):CONFIG(desktop) {
    SUBDIRS *= skin_generator
  }

  CONFIG(desktop) {
    drape_head.depends = $$SUBDIRS
    SUBDIRS *= drape_head
  }

  CONFIG(desktop) {
    benchmark_tool.subdir = map/benchmark_tool
    benchmark_tool.depends = 3party base coding geometry platform indexer search map
    mapshot.depends = $$SUBDIRS
    qt.depends = $$SUBDIRS

    SUBDIRS *= benchmark_tool mapshot qt
    }

  CONFIG(desktop) {
    # Desktop-only support library, used in tests and search quality tools.
    generator_tests_support.subdir = generator/generator_tests_support
    SUBDIRS *= generator_tests_support
  }

  CONFIG(desktop) {
    # Desktop-only support library, used in tests and search quality tools.
    search_tests_support.subdir = search/search_tests_support
    SUBDIRS *= search_tests_support

    search_quality.subdir = search/search_quality
    search_quality.depends = $$SUBDIRS
    SUBDIRS *= search_quality

    search_quality_tool.subdir = search/search_quality/search_quality_tool
    search_quality_tool.depends = $$SUBDIRS

    features_collector_tool.subdir = search/search_quality/features_collector_tool
    features_collector_tool.depends = $$SUBDIRS

    SUBDIRS *= search_quality_tool features_collector_tool
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
    geometry_tests.depends = 3party base geometry
    SUBDIRS *= geometry_tests

    indexer_tests.subdir = indexer/indexer_tests
    indexer_tests.depends = 3party base coding geometry indexer editor
    SUBDIRS *= indexer_tests

    platform_tests.subdir = platform/platform_tests
    platform_tests.depends = 3party base coding platform platform_tests_support
    SUBDIRS *= platform_tests

    downloader_tests.subdir = platform/downloader_tests
    downloader_tests.depends = 3party base coding platform platform_tests_support
    SUBDIRS *= downloader_tests

    search_tests.subdir = search/search_tests
    search_tests.depends = 3party base coding geometry platform indexer search
    SUBDIRS *= search_tests

    MapDepLibs = 3party base coding geometry editor platform storage indexer search map \
                 routing drape drape_frontend

    # @TODO storage_tests.depends is equal to map_tests because now storage/migrate_tests.cpp
    # is located in storage_tests. When the new map downloader is used and storage_integraion_tests
    # is recovered storage/migrate_tests.cpp should be moved to storage_integraion_tests
    # storage_tests.depends should be set to |3party base coding geometry platform storage indexer stats|
    # as it was before.
    storage_tests.subdir = storage/storage_tests
    storage_tests.depends = $$MapDepLibs generator_tests_support generator
    SUBDIRS *= storage_tests

    storage_integration_tests.subdir = storage/storage_integration_tests
    storage_integration_tests.depends = $$MapDepLibs
    SUBDIRS *= storage_integration_tests

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
    routing_tests.depends = $$MapDepLibs
    SUBDIRS *= routing_tests

    routing_integration_tests.subdir = routing/routing_integration_tests
    routing_integration_tests.depends = $$MapDepLibs routing
    SUBDIRS *= routing_integration_tests

    routing_consistency_tests.subdir = routing/routing_consistency_tests
    routing_consistency_tests.depends = $$MapDepLibs routing
    SUBDIRS *= routing_consistency_tests

    srtm_source_tests.subdir = generator/srtm_source_tests
    srtm_source_tests.depends = $$MapDepLibs routing
    SUBDIRS *= srtm_source_tests

    # TODO(AlexZ): Move pedestrian tests into routing dir.
    pedestrian_routing_tests.depends = $$MapDepLibs routing
    SUBDIRS *= pedestrian_routing_tests

    search_tests_support.subdir = search/search_tests_support
    search_tests_support.depends = $$MapDepLibs
    SUBDIRS *= search_tests_support

    search_integration_tests.subdir = search/search_integration_tests
    search_integration_tests.depends = $$MapDepLibs search_tests_support \
                                       generator_tests_support generator
    SUBDIRS *= search_integration_tests

    search_quality_tests.subdir = search/search_quality/search_quality_tests
    search_quality_tests.depends = $$MapDepLibs search_quality search_tests_support
    SUBDIRS *= search_quality_tests

    generator_tests.subdir = generator/generator_tests
    generator_tests.depends = $$MapDepLibs routing generator
    SUBDIRS *= generator_tests

    editor_tests.subdir = editor/editor_tests
    editor_tests.depends = 3party base coding geometry editor
    SUBDIRS *= editor_tests

    SUBDIRS *= qt_tstfrm

    drape_tests.subdir = drape/drape_tests
    drape_tests.depends = 3party base coding platform qt_tstfrm
    SUBDIRS *= drape_tests

    drape_frontend_tests.subdir = drape_frontend/drape_frontend_tests
    drape_frontend_tests.depends = 3party base coding platform drape drape_frontend
    SUBDIRS *= drape_frontend_tests
  } # !no-tests
} # !gtool
