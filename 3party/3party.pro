# Project that includes all third party projects.

TEMPLATE = subdirs

SUBDIRS = freetype fribidi zlib bzip2 jansson tomcrypt protobuf osrm expat \
    succinct opening_hours \


!iphone*:!tizen*:!android* {
  SUBDIRS += gflags   \
             sgitess  \
             qjsonrpc \
             o5mreader\
             gmock    \
}
