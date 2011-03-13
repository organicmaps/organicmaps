# Project that includes all third party projects.

TEMPLATE = subdirs

SUBDIRS = expat freetype fribidi zlib bzip2 jansson protobuf tomcrypt

!iphonesimulator-g++42 {
  !iphonedevice-g++42 {
    !bada-simulator {
      SUBDIRS += gflags \
                 sgitess
      }
   }
}
