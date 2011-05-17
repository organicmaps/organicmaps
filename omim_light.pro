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
          search search/search_tests \
          map \
          qt_tstfrm \
          indexer/indexer_tests \
          map/map_tests \
          generator \
          generator/generator_tests \
          generator/generator_tool \
          yg/yg_tests \
          qt
} else {
  # libraries which are used on iphone and bada
  message = "Please use omim.pro for iphone builds"
}

