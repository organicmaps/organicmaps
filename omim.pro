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
#CONFIG += drape_device

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
          qt_tstfrm \
          drape \
          drape/drape_tests \
          graphics \
          gui \
          render render/render_tests \
          storage storage/storage_tests \
          search search/search_tests \
          drape_frontend drape_frontend/drape_frontend_tests \
          map map/map_tests map/benchmark_tool map/mwm_tests\
          generator generator/generator_tests \
          generator/generator_tool \
          indexer/indexer_tests \
          graphics/graphics_tests \
          gui/gui_tests \
          qt \
          drape_head \
          map_server \
          integration_tests \
          pedestrian_routing_benchmarks \
          search/integration_tests \
} else:drape_device {
  # libraries which are used on mobile devices with drape engine
  SUBDIRS = 3party \
            base \
            coding \
            geometry \
            drape \
            platform \
            anim \
            indexer \
            routing \
            storage \
            graphics \
            gui \
            render \
            search \
            drape_frontend \
            map \
            stats \
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
            render \
            search \
            map \
            stats \
}

win32 {
  SUBDIRS -= \
    drape drape/drape_tests \
    drape_frontend drape_frontend/drape_frontend_tests \
    drape_head
}
