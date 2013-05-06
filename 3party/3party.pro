# Project that includes all third party projects.

TEMPLATE = subdirs

SUBDIRS = expat freetype fribidi zlib bzip2 jansson tomcrypt protobuf sqlite3

!iphone*:!bada*:!android* {
  SUBDIRS += gflags \
             sgitess \
             sqlite3/sqlite3_tests
}
