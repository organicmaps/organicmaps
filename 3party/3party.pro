# Project that includes all third party projects.

TEMPLATE = subdirs

SUBDIRS = freetype fribidi zlib bzip2 jansson tomcrypt protobuf

!linux*:android* {
  SUBDIRS *= expat
}

!iphone*:!bada*:!android* {
  SUBDIRS += gflags \
             sgitess \
             qjsonrpc
}
