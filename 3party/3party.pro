# Project that includes all third party projects.

TEMPLATE = subdirs

SUBDIRS = freetype fribidi zlib bzip2 jansson tomcrypt protobuf

# use expat from the system on linux
!linux*: SUBDIRS *= expat

!iphone*:!bada*:!android* {
  SUBDIRS += gflags   \
             sgitess  \
             qjsonrpc \
             gmock    \
}
