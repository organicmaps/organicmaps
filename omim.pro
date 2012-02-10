# Project that just includes all other projects.

TEMPLATE = subdirs
CONFIG += ordered

HEADERS += defines.hpp

# desktop projects
!iphone*:!bada*:!android* {
SUBDIRS = 3party \
          base base/base_tests \
          coding coding/coding_tests \
          geometry \
          version \
          indexer \
          platform \
          geometry/geometry_tests \
          platform/platform_tests \
          yg \
          storage storage/storage_tests \
          search search/search_tests \
          map map/map_tests map/benchmark_tool \
          generator generator/generator_tests \
          generator/generator_tool \
          qt_tstfrm \
          indexer/indexer_tests \
          yg/yg_tests \
          qt
} else {
  # libraries which are used on mobile devices
  SUBDIRS = 3party \
            base \
            coding \
            geometry \
            version \
            platform \
            indexer \
            storage \
            yg \
            search \
            map
}
