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
!iphone*:!tizen*:!android* {
SUBDIRS = 3party \
          base base/base_tests \
          coding coding/coding_tests \
          geometry \
          stats \
          indexer \
          platform \
          routing routing/routing_tests \
          geometry/geometry_tests \
          platform/platform_tests \
          anim \
          drape \
          drape/drape_tests \
          graphics \
          gui \
          storage storage/storage_tests \
          search search/search_tests \
          map map/map_tests map/benchmark_tool map/mwm_tests\
          drape_frontend drape_frontend/drape_frontend_tests \
          generator generator/generator_tests \
          generator/generator_tool \
          qt_tstfrm \
          indexer/indexer_tests \
          graphics/graphics_tests \
          gui/gui_tests \
          qt \
          drape_head \
          map_server \
          yopme_desktop
} else {
  # libraries which are used on mobile devices
  SUBDIRS = 3party \
            base \
            coding \
            geometry \
            platform \
            anim \
            indexer \
            routing \
            storage \
            graphics \
            gui \
            search \
            map \
            stats/client \
}
