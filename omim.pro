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
          indexer \
          platform \
          geometry/geometry_tests \
          platform/platform_tests \
          anim \
          graphics \
          gui \
          storage storage/storage_tests \
          search search/search_tests \
          map map/map_tests map/benchmark_tool map/mwm_tests\
          generator generator/generator_tests \
          generator/generator_tool \
          qt_tstfrm \
          indexer/indexer_tests \
          graphics/graphics_tests \
          gui/gui_tests \
          qt
} else {
  # libraries which are used on mobile devices
  SUBDIRS = 3party \
            base \
            coding \
            geometry \
            platform \
            anim \
            indexer \
            storage \
            graphics \
            gui \
            search \
            map \
}
