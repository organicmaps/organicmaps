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

CONFIG(map_designer_standalone) {
  CONFIG += map_designer
}

SUBDIRS = 3party base coding geometry editor ugc indexer routing routing_common search openlr

!CONFIG(osrm) {
  SUBDIRS *= platform stats storage map

  mwm_diff.subdir = generator/mwm_diff
  SUBDIRS *= mwm_diff

  CONFIG(desktop) {
    SUBDIRS *= traffic generator

    generator_tool.subdir = generator/generator_tool
    generator_tool.depends = $$SUBDIRS
    SUBDIRS *= generator_tool

    openlr_stat.subdir = openlr/openlr_stat
    openlr_stat.depends = $$SUBDIRS
    SUBDIRS *= openlr_stat
  }

  # Integration tests dependencies for gtool.
  # TODO(AlexZ): Avoid duplication for routing_integration_tests.
  CONFIG(gtool):!CONFIG(no-tests) {
    # Booking quality check
    booking_quality_check.subdir = generator/booking_quality_check
    booking_quality_check.depends = $$SUBDIRS
    SUBDIRS *= booking_quality_check

    # restaraunts_info
    restaurants_info.subdir = generator/restaurants_info
    restaurants_info.depends = $$SUBDIRS
    SUBDIRS *= restaurants_info

    routing_integration_tests.subdir = routing/routing_integration_tests
    routing_integration_tests.depends = $$SUBDIRS
    routing_consistency_tests.subdir = routing/routing_consistency_tests
    routing_consistency_tests.depends = $$SUBDIRS
    srtm_coverage_checker.subdir = generator/srtm_coverage_checker
    srtm_coverage_checker.depends = $$SUBDIRS routing
    feature_segments_checker.subdir = generator/feature_segments_checker
    feature_segments_checker.depends = $$SUBDIRS
    SUBDIRS *= routing_integration_tests routing_consistency_tests srtm_coverage_checker feature_segments_checker
  }
}

!CONFIG(gtool):!CONFIG(osrm) {
  SUBDIRS *= drape drape_frontend partners_api local_ads tracking traffic

  CONFIG(map_designer):CONFIG(desktop) {
    skin_generator.depends = $$SUBDIRS
    SUBDIRS *= skin_generator
  }

  CONFIG(desktop) {
    benchmark_tool.subdir = map/benchmark_tool
    benchmark_tool.depends = 3party base coding geometry platform indexer search map

    qt_common.subdir = qt/qt_common
    qt_common.depends = $$SUBDIRS

    qt.depends = $$SUBDIRS qt_common

    SUBDIRS *= benchmark_tool qt qt_common
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

    feature_list.subdir = feature_list
    feature_list.depends = $$SUBDIRS
    SUBDIRS *= feature_list

    search_quality_tool.subdir = search/search_quality/search_quality_tool
    search_quality_tool.depends = $$SUBDIRS

    features_collector_tool.subdir = search/search_quality/features_collector_tool
    features_collector_tool.depends = $$SUBDIRS
    SUBDIRS *= features_collector_tool search_quality_tool
  }

  CONFIG(map_designer):CONFIG(no-tests) {
    # Designer Tool package includes style tests
    style_tests.subdir = map/style_tests
    style_tests.depends = 3party base coding geometry editor platform storage indexer search map \
                          routing_common drape drape_frontend
    SUBDIRS *= style_tests
  }

  CONFIG(desktop):!CONFIG(no-tests) {
    # Additional desktop-only, tests-only libraries.
    platform_tests_support.subdir = platform/platform_tests_support
    SUBDIRS *= platform_tests_support

    indexer_tests_support.subdir = indexer/indexer_tests_support
    SUBDIRS *= indexer_tests_support

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
    indexer_tests.depends = 3party base coding geometry platform editor storage routing indexer \
                            platform_tests_support search_tests_support generator_tests_support \
                            indexer_tests_support

    SUBDIRS *= indexer_tests

    platform_tests.subdir = platform/platform_tests
    platform_tests.depends = 3party base coding platform platform_tests_support
    SUBDIRS *= platform_tests

#   Uncomment after replace hash function
#    downloader_tests.subdir = platform/downloader_tests
#    downloader_tests.depends = 3party base coding platform platform_tests_support
#    SUBDIRS *= downloader_tests

    search_tests.subdir = search/search_tests
    search_tests.depends = 3party base coding geometry platform indexer search \
                           search_tests_support indexer_tests_support generator_tests_support
    SUBDIRS *= search_tests

    MapDepLibs = 3party base coding geometry editor platform storage indexer search map \
                 routing_common drape drape_frontend

    # @TODO storage_tests.depends is equal to map_tests because now storage/migrate_tests.cpp
    # is located in storage_tests. When the new map downloader is used and storage_integraion_tests
    # is recovered storage/migrate_tests.cpp should be moved to storage_integraion_tests
    # storage_tests.depends should be set to |3party base coding geometry platform storage indexer stats|
    # as it was before.
    storage_tests.subdir = storage/storage_tests
    storage_tests.depends = $$MapDepLibs generator_tests_support generator partners_api
    SUBDIRS *= storage_tests

    storage_integration_tests.subdir = storage/storage_integration_tests
    storage_integration_tests.depends = $$MapDepLibs partners_api
    SUBDIRS *= storage_integration_tests

    map_tests.subdir = map/map_tests
    map_tests.depends = $$MapDepLibs search_tests_support
    SUBDIRS *= map_tests

    mwm_tests.subdir = map/mwm_tests
    mwm_tests.depends = $$MapDepLibs
    SUBDIRS *= mwm_tests

    style_tests.subdir = map/style_tests
    style_tests.depends = $$MapDepLibs
    SUBDIRS *= style_tests

    routing_common_tests.subdir = routing_common/routing_common_tests
    routing_common_tests.depends = $$MapDepLibs
    SUBDIRS *= routing_common_tests

    routing_tests.subdir = routing/routing_tests
    routing_tests.depends = $$MapDepLibs routing
    SUBDIRS *= routing_tests

    routing_integration_tests.subdir = routing/routing_integration_tests
    routing_integration_tests.depends = $$MapDepLibs routing
    SUBDIRS *= routing_integration_tests

    routing_consistency_tests.subdir = routing/routing_consistency_tests
    routing_consistency_tests.depends = $$MapDepLibs routing generator
    SUBDIRS *= routing_consistency_tests

    srtm_coverage_checker.subdir = generator/srtm_coverage_checker
    srtm_coverage_checker.depends = $$MapDepLibs routing
    SUBDIRS *= srtm_coverage_checker

    feature_segments_checker.subdir = generator/feature_segments_checker
    feature_segments_checker.depends = $$MapDepLibs routing
    SUBDIRS *= feature_segments_checker

    routing_benchmarks.subdir = routing/routing_benchmarks
    routing_benchmarks.depends = $$MapDepLibs routing
    SUBDIRS *= routing_benchmarks

    search_tests_support.subdir = search/search_tests_support
    search_tests_support.depends = $$MapDepLibs
    SUBDIRS *= search_tests_support

    search_integration_tests.subdir = search/search_integration_tests
    search_integration_tests.depends = $$MapDepLibs \
                                       search_tests_support generator_tests_support indexer_tests_support generator
    SUBDIRS *= search_integration_tests

    search_quality_tests.subdir = search/search_quality/search_quality_tests
    search_quality_tests.depends = $$MapDepLibs search_quality search_tests_support
    SUBDIRS *= search_quality_tests

    generator_tests.subdir = generator/generator_tests
    generator_tests.depends = $$MapDepLibs routing generator
    SUBDIRS *= generator_tests

    editor_tests.subdir = editor/editor_tests
    editor_tests.depends = 3party base coding geometry platform editor
    SUBDIRS *= editor_tests

    osm_auth_tests.subdir = editor/osm_auth_tests
    osm_auth_tests.depends = 3party base coding geometry platform editor
    SUBDIRS *= osm_auth_tests

    SUBDIRS *= qt_tstfrm

    drape_tests.subdir = drape/drape_tests
    drape_tests.depends = 3party base coding platform qt_tstfrm
    SUBDIRS *= drape_tests

    drape_frontend_tests.subdir = drape_frontend/drape_frontend_tests
    drape_frontend_tests.depends = 3party base coding platform drape drape_frontend
    SUBDIRS *= drape_frontend_tests

    partners_api_tests.subdir = partners_api/partners_api_tests
    partners_api_tests.depends = 3party base geometry coding platform indexer partners_api
    SUBDIRS *= partners_api_tests

    local_ads_tests.subdir = local_ads/local_ads_tests
    local_ads_tests.depends = base local_ads platform_tests_support platform coding
    SUBDIRS *= local_ads_tests

    tracking_tests.subdir = tracking/tracking_tests
    tracking_tests.depends = 3party base routing routing_common tracking platform_tests_support platform coding geometry
    SUBDIRS *= tracking_tests

    traffic_tests.subdir = traffic/traffic_tests
    traffic_tests.depends = 3party base traffic routing_common platform_tests_support platform coding geometry
    SUBDIRS *= traffic_tests

    openlr_tests.subdir = openlr/openlr_tests
    openlr_tests.depends = $$SUBDIRS platform_tests_support
    SUBDIRS *= openlr_tests

    ugc_tests.subdir = ugc/ugc_tests
    ugc_tests.depends = ugc indexer platform coding geometry base generator generator_tests_support 3party routing \
                        routing_common search editor storage
    SUBDIRS *= ugc_tests

    mwm_diff_tests.subdir = generator/mwm_diff/mwm_diff_tests
    mwm_diff_tests.depends = mwm_diff generator platform coding base
    SUBDIRS *= mwm_diff_tests
  } # !no-tests

  CONFIG(map_shot) {
    mapshot.depends = 3party base coding geometry editor platform storage indexer search map \
                      routing_common drape drape_frontend software_renderer
    SUBDIRS *= software_renderer mapshot
  }
} # !gtool
