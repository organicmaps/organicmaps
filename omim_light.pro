# Project that just includes all other projects, except 3party, publisher, sloynik.

TEMPLATE = subdirs
CONFIG += ordered

HEADERS += defines.hpp

# desktop projects
win32|macx {
SUBDIRS = \
          base base/base_tests \
          coding coding/coding_tests \
          geometry \
          indexer \
          platform \
          stats \
          search search/search_tests \
          geometry/geometry_tests \
          platform/platform_tests \
          yg \
          storage storage/storage_tests \
          map map/map_tests map/benchmark_tool \
          generator generator/generator_tests \
          generator/generator_tool \
          qt_tstfrm \
          indexer/indexer_tests \
          yg/yg_tests \
          words words/words_tests \
          qt \
} else {
  # libraries which are used on iphone and bada
  message = "Please use omim.pro for iphone builds"
}

