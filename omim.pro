# Build file for MAPS.ME project
#
# Possible options:
#   3party: build only 3party libraries
#   gtool: build only generator_tool
#   skin-gen: build skin-generator tool
#   no-tests: do not build tests for desktop
#   drape: include drape libraries (implies no-tests)
#   iphone / tizen / android: build an app (implies no-tests)

lessThan(QT_MAJOR_VERSION, 5) {
  error("You need Qt 5 to build this project. You have Qt $$QT_VERSION")
}

cache()

TEMPLATE = subdirs
CONFIG += ordered

HEADERS += defines.hpp

win32:CONFIG(drape) {
  CONFIG -= drape
}

!iphone*:!tizen*:!android* {
  CONFIG *= desktop
}

CONFIG(designer) {
  DEFINES *= BUILD_DESIGNER
  CONFIG *= skin-gen
}

SUBDIRS = 3party

!CONFIG(3party) {
  SUBDIRS += base geometry coding

  SUBDIRS += platform
  SUBDIRS += stats
  SUBDIRS += indexer
  SUBDIRS += routing
  SUBDIRS += storage

  CONFIG(desktop) {
    SUBDIRS += generator generator/generator_tool
  }

  !CONFIG(gtool) {
    SUBDIRS += anim
    SUBDIRS += graphics
    SUBDIRS += gui
    SUBDIRS += render
    SUBDIRS += search
    SUBDIRS += map

    CONFIG(desktop) {
      SUBDIRS += qt
    }

    CONFIG(skin-gen) {
      SUBDIRS += skin_generator
    }

    CONFIG(drape) {
      SUBDIRS += drape drape_frontend

      CONFIG(desktop) {
        SUBDIRS += drape_head
      }
    }

    CONFIG(desktop):!CONFIG(no-tests) {
      SUBDIRS += base/base_tests
      SUBDIRS += coding/coding_tests
      SUBDIRS += platform/platform_tests_support
      SUBDIRS += geometry/geometry_tests
      SUBDIRS += platform/platform_tests
      SUBDIRS += qt_tstfrm
      SUBDIRS += render/render_tests
      SUBDIRS += storage/storage_tests
      SUBDIRS += search/search_tests
      SUBDIRS += map/map_tests map/benchmark_tool map/mwm_tests
      SUBDIRS += routing/routing_tests
      SUBDIRS += generator/generator_tests
      SUBDIRS += indexer/indexer_tests
      SUBDIRS += graphics/graphics_tests
      SUBDIRS += gui/gui_tests
      SUBDIRS += integration_tests
      SUBDIRS += pedestrian_routing_benchmarks
      SUBDIRS += search/integration_tests

      CONFIG(drape) {
        SUBDIRS += drape/drape_tests
        SUBDIRS += drape_frontend/drape_frontend_tests
      }
    } # !no-tests
  } # !gtool
} # !3party
