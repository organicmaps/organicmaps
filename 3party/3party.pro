# Projct that includes all third party projects.

TEMPLATE = subdirs

SUBDIRS = expat freetype

!iphonesimulator-g++42 {
  !iphonedevice-g++42 {
    !bada-simulator {
      SUBDIRS += gflags \
                 sgitess
      }
   }
}
