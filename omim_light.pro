# Project that just includes all other projects, except 3party.

TEMPLATE = subdirs
CONFIG += ordered

# desktop projects
win32|macx {
SUBDIRS = \
          base base/base_tests \
          coding coding/coding_tests \
          geometry geometry/geometry_tests \
          platform platform/platform_tests \
          yg \
          storage storage/storage_tests \
          indexer \
          map \
          qt_tstfrm \
          indexer/indexer_tests \
          map/map_tests \
          indexer/indexer_tool \
          yg/yg_tests \
          qt
}

# libraries which are used on iphone and bada
iphonesimulator-g++42|iphonedevice-g++42 |
bada-simulator|bada-device {
  message = "Please use omim.pro for iphone builds"
}

