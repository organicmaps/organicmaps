# Project that includes all third party projects.

TEMPLATE = subdirs

# TODO: Avoid duplication here and in omim.pro by moving opening_hours tests out of 3party.
!iphone*:!tizen*:!android* {
  CONFIG *= desktop
}

SUBDIRS = freetype minizip jansson protobuf osrm expat succinct pugixml liboauthcpp stb_image sdf_image \
    icu

# TODO(mgsrergio): Move opening hours out of 3party to the main project tree.
# See https://trello.com/c/tWYSnXSS/22-opening-hours-3party-boost-test-framework.
SUBDIRS *= opening_hours
# Disable tests for gtool profile, since it needs only routing tests.
CONFIG(desktop):!CONFIG(no-tests):!CONFIG(gtool):!CONFIG(osrm) {
  opening_hours_tests.subdir = opening_hours/opening_hours_tests
  opening_hours_tests.depends = opening_hours
  SUBDIRS *= opening_hours_tests

  opening_hours_integration_tests.subdir = opening_hours/opening_hours_integration_tests
  opening_hours_integration_tests.depends = opening_hours
  SUBDIRS *= opening_hours_integration_tests

  opening_hours_supported_features_tests.subdir = opening_hours/opening_hours_supported_features_tests
  opening_hours_supported_features_tests.depends = opening_hours
  SUBDIRS *= opening_hours_supported_features_tests
}

CONFIG(desktop) {
  SUBDIRS *= gflags libtess2 gmock
}
