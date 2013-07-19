# Project that includes all third party projects.

TEMPLATE = subdirs

SUBDIRS = expat freetype fribidi zlib bzip2 jansson tomcrypt protobuf qjsonrpc

!iphone*:!bada*:!android* {
  SUBDIRS += gflags \
             sgitess
}
