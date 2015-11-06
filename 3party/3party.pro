# Project that includes all third party projects.

TEMPLATE = subdirs

# TODO: Avoid duplication here and in omim.pro by moving
# opening_hours tests out of 3party.
!iphone*:!tizen*:!android* {
  CONFIG *= desktop
}

SUBDIRS = freetype fribidi minizip jansson tomcrypt protobuf osrm expat \
    succinct \

SUBDIRS += opening_hours
CONFIG(desktop):!CONFIG(no-tests) {
  SUBDIRS += opening_hours/opening_hours_tests \
             opening_hours/opening_hours_integration_tests \
             opening_hours/opening_hours_supported_features_tests
}

CONFIG(desktop) {
  SUBDIRS += gflags   \
             libtess2  \
             gmock
}
