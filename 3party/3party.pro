# Project that includes all third party projects.

TEMPLATE = subdirs

SUBDIRS = freetype fribidi minizip jansson tomcrypt protobuf osrm expat \
    succinct \

!linux* {
  SUBDIRS += opening_hours
  CONFIG(desktop):!CONFIG(no-tests) {
    SUBDIRS += opening_hours/opening_hours_tests \
               opening_hours/opening_hours_integration_tests \
               opening_hours/opening_hours_supported_features_tests
  }
}

!iphone*:!tizen*:!android* {
  SUBDIRS += gflags   \
             libtess2  \
             gmock
}
