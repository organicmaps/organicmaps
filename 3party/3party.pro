# Project that includes all third party projects.

TEMPLATE = subdirs

SUBDIRS = freetype fribidi minizip jansson tomcrypt protobuf osrm expat \
    succinct \

!linux* {
SUBDIRS += opening_hours \
           opening_hours/opening_hours_tests \
}

!iphone*:!tizen*:!android* {
  SUBDIRS += gflags   \
             libtess2  \
             gmock    \
}
