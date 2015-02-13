# Project that includes all third party projects.

TEMPLATE = subdirs

SUBDIRS = freetype fribidi zlib bzip2 jansson tomcrypt protobuf osrm expat \
    o5mreader

!iphone*:!tizen*:!android* {
  SUBDIRS += gflags   \
             sgitess  \
             qjsonrpc \
             gmock    \
}
