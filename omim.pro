# Project that just includes all other projects.

TEMPLATE = subdirs
CONFIG += ordered

HEADERS += defines.hpp

# desktop projects
win32|macx|unix {
SUBDIRS = 3party \
          base base/base_tests \
          coding coding/coding_tests \
          geometry geometry/geometry_tests \
          platform platform/platform_tests \
          yg \
          indexer \
          storage storage/storage_tests \
          map \
          map/map_tests \
          version \
          indexer/indexer_tool \
          qt_tstfrm \
          indexer/indexer_tests \
          yg/yg_tests \
          qt
}

# libraries which are used on iphone and bada
iphonesimulator-g++42|iphonedevice-g++42 {
  SUBDIRS = 3party \
            base \
            coding \
            geometry \
            yg \
            indexer \
            storage \
            map \
            version
}
