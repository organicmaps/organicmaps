# Project that includes all third party projects.

TEMPLATE = subdirs

SUBDIRS = expat freetype fribidi zlib bzip2 jansson protobuf tomcrypt

!iphone*:!bada* {
  SUBDIRS += gflags \
             sgitess
}
