# Project that just includes all other projects.
QT_VERSION = $$[QT_VERSION]
QT_VERSION = $$split(QT_VERSION, ".")
QT_VER_MAJ = $$member(QT_VERSION, 0)
QT_VER_MIN = $$member(QT_VERSION, 1)

greaterThan(QT_VER_MAJ, 4) {
  cache()
}

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
          qt \
          map_server map_server/map_server_tests \
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
