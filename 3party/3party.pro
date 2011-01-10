# Project that includes all third party projects.

TEMPLATE = subdirs

SUBDIRS = expat freetype fribidi

!iphonesimulator-g++42 {
  !iphonedevice-g++42 {
    !bada-simulator {
      SUBDIRS += gflags \
                 sgitess
      }
   }
}
