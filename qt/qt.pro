# Main application in qt.
ROOT_DIR = ..

DEPENDENCIES = qt_common map drape_frontend openlr routing search storage tracking traffic \
               routing_common ugc indexer drape partners_api local_ads platform editor mwm_diff \
               geometry coding base freetype expat gflags jansson protobuf osrm stats_client \
               minizip succinct pugixml oauthcpp stb_image sdf_image icu

DEPENDENCIES += opening_hours \

include($$ROOT_DIR/common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/gflags/src

map_designer {
  TARGET = MAPS.ME.Designer
} else {
  TARGET = MAPS.ME
}

map_designer {
  RM_MDPI = $$system(rm -rf $$PWD/../data/resources-mdpi_design)
  RM_HDPI = $$system(rm -rf $$PWD/../data/resources-hdpi_design)
  RM_XHDPI = $$system(rm -rf $$PWD/../data/resources-xhdpi_design)
  RM_XXHDPI = $$system(rm -rf $$PWD/../data/resources-xxhdpi_design)
  RM_6PLUS = $$system(rm -rf $$PWD/../data/resources-6plus_design)
  CP_MDPI = $$system(cp -rf $$PWD/../data/resources-mdpi_clear $$PWD/../data/resources-mdpi_design)
  CP_HDPI = $$system(cp -rf $$PWD/../data/resources-hdpi_clear $$PWD/../data/resources-hdpi_design)
  CP_XHDPI = $$system(cp -rf $$PWD/../data/resources-xhdpi_clear $$PWD/../data/resources-xhdpi_design)
  CP_XXHDPI = $$system(cp -rf $$PWD/../data/resources-xxhdpi_clear $$PWD/../data/resources-xxhdpi_design)
  CP_6PLUS = $$system(cp -rf $$PWD/../data/resources-6plus_clear $$PWD/../data/resources-6plus_design)
  CP_DRULES = $$system(cp -f $$PWD/../data/drules_proto_clear.bin $$PWD/../data/drules_proto_design.bin)
  CP_COLORS = $$system(cp -f $$PWD/../data/colors.txt $$PWD/../data/colors_design.txt)
  CP_PATTERNS = $$system(cp -f $$PWD/../data/patterns.txt $$PWD/../data/patterns_design.txt)
}

TEMPLATE = app
CONFIG += warn_on
QT *= core widgets gui opengl

win32* {
  LIBS *= -lopengl32 -lws2_32 -liphlpapi
  RC_FILE = res/windows.rc
  win32-msvc*: LIBS *= -lwlanapi
}

win32*|linux* {
  QT *= network
}

linux* {
  isEmpty(PREFIX):PREFIX = /usr
  DEFINES += INSTALL_PREFIX=$$(PREFIX)
  BINDIR = $$PREFIX/bin

  DATADIR = $$PREFIX/share/mapsme
  FONTSDIR = /usr/share/fonts/truetype/mapsme/

  target.path = $$BINDIR
  desktop.path = /usr/share/applications/
  desktop.files += res/$${TARGET}.desktop

  INSTALLS += target desktop
}

macx-* {
  LIBS *= "-framework CoreLocation" "-framework CoreWLAN" \
          "-framework QuartzCore" "-framework IOKit" "-framework SystemConfiguration"

  map_designer {
    ICON = res/designer.icns
  } else {
    ICON = res/mac.icns
  }

  PLIST_FILE = Info.plist
  # path to original plist, which will be processed by qmake and later by us
  QMAKE_INFO_PLIST = res/$${PLIST_FILE}

  # fix version directly in bundle's Info.plist
  PLIST_PATH = $${DESTDIR}/$${TARGET}.app/Contents/$${PLIST_FILE}
  QMAKE_POST_LINK = $${IN_PWD}/../tools/unix/process_plist.sh $${IN_PWD}/.. $$VERSION_MAJOR $$VERSION_MINOR $$PLIST_PATH

  DATADIR = Contents/Resources
  FONTSDIR = $$DATADIR
}

OTHER_RES.path = $$DATADIR
OTHER_RES.files = ../data/copyright.html ../data/eula.html ../data/welcome.html \
                  ../data/countries.txt ../data/colors.txt ../data/patterns.txt \
                  ../data/languages.txt ../data/categories.txt \
                  ../data/packed_polygons.bin res/logo.png \
                  ../data/editor.config \
                  ../data/local_ads_symbols.txt \

CLASSIFICATOR_RES.path = $$DATADIR
CLASSIFICATOR_RES.files = ../data/classificator.txt \
                          ../data/types.txt \
                          ../data/drules_proto_clear.bin

DEFAULT_SKIN_RES.path = $$DATADIR/resources-default
DEFAULT_SKIN_RES.files = ../resources-default/default.ui

!map_designer {
  MDPI_SKIN_RES.path = $$DATADIR/resources-mdpi_clear
  MDPI_SKIN_RES.files = ../data/resources-mdpi_clear/

  XHDPI_SKIN_RES.path = $$DATADIR/resources-xhdpi_clear
  XHDPI_SKIN_RES.files = ../data/resources-xhdpi_clear/
}

map_designer {
  DESIGN_TOOL_RES.path = $$DATADIR
  DESIGN_TOOL_RES.files = ../data/drules_proto_design.bin \
                          ../data/colors_design.txt \
                          ../data/patterns_design.txt \
                          ../data/mapcss-dynamic.txt \
                          ../data/mapcss-mapping.csv

  DESIGN_TOOL_MDPI_RES.path = $$DATADIR/resources-mdpi_design
  DESIGN_TOOL_MDPI_RES.files = ../data/resources-mdpi_design/

  DESIGN_TOOL_HDPI_RES.path = $$DATADIR/resources-hdpi_design
  DESIGN_TOOL_HDPI_RES.files = ../data/resources-hdpi_design/

  DESIGN_TOOL_XHDPI_RES.path = $$DATADIR/resources-xhdpi_design
  DESIGN_TOOL_XHDPI_RES.files = ../data/resources-xhdpi_design/

  DESIGN_TOOL_XXHDPI_RES.path = $$DATADIR/resources-xxhdpi_design
  DESIGN_TOOL_XXHDPI_RES.files = ../data/resources-xxhdpi_design/

  DESIGN_TOOL_6PLUS_RES.path = $$DATADIR/resources-6plus_design
  DESIGN_TOOL_6PLUS_RES.files = ../data/resources-6plus_design/

  COUNTRIES_STRINGS_RES.path = $$DATADIR/countries-strings
  COUNTRIES_STRINGS_RES.files = ../data/countries-strings/
}

FONT_RES.path = $$FONTSDIR
FONT_RES.files = ../data/01_dejavusans.ttf \
                 ../data/02_droidsans-fallback.ttf \
                 ../data/03_jomolhari-id-a3d.ttf \
                 ../data/04_padauk.ttf \
                 ../data/05_khmeros.ttf \
                 ../data/06_code2000.ttf \
                 ../data/07_roboto_medium.ttf

OTHER_RES.files += ../data/fonts_blacklist.txt \
                   ../data/fonts_whitelist.txt \
                   ../data/unicode_blocks.txt

ICU_RES.path = $$DATADIR
ICU_RES.files = ../data/icudt57l.dat

MWM_RES.path = $$DATADIR
MWM_RES.files = ../data/World.mwm ../data/WorldCoasts.mwm

ALL_RESOURCES = OTHER_RES CLASSIFICATOR_RES MDPI_SKIN_RES XHDPI_SKIN_RES FONT_RES MWM_RES ICU_RES
map_designer {
  ALL_RESOURCES += DESIGN_TOOL_RES DESIGN_TOOL_MDPI_RES DESIGN_TOOL_HDPI_RES DESIGN_TOOL_XHDPI_RES \
                   DESIGN_TOOL_XXHDPI_RES DESIGN_TOOL_6PLUS_RES COUNTRIES_STRINGS_RES
}
#ALL_RESOURCES += DEFAULT_SKIN_RES

linux* {
  INSTALLS += $$ALL_RESOURCES
}

macx-* {
  QMAKE_BUNDLE_DATA += $$ALL_RESOURCES
}

map_designer {
SOURCES += \
    build_style/build_common.cpp \
    build_style/build_drules.cpp \
    build_style/build_phone_pack.cpp \
    build_style/build_skins.cpp \
    build_style/build_style.cpp \
    build_style/build_statistics.cpp \
    build_style/run_tests.cpp \

HEADERS += \
    build_style/build_common.h \
    build_style/build_drules.h \
    build_style/build_phone_pack.h \
    build_style/build_skins.h \
    build_style/build_style.h \
    build_style/build_statistics.h \
    build_style/run_tests.h \

}

SOURCES += \
    about.cpp \
    create_feature_dialog.cpp \
    draw_widget.cpp \
    editor_dialog.cpp \
    info_dialog.cpp \
    main.cpp \
    mainwindow.cpp \
    osm_auth_dialog.cpp \
    place_page_dialog.cpp \
    preferences_dialog.cpp \
    search_panel.cpp \
    update_dialog.cpp \

HEADERS += \
    about.hpp \
    create_feature_dialog.hpp \
    draw_widget.hpp \
    editor_dialog.hpp \
    info_dialog.hpp \
    mainwindow.hpp \
    osm_auth_dialog.hpp \
    place_page_dialog.hpp \
    preferences_dialog.hpp \
    search_panel.hpp \
    update_dialog.hpp \

RESOURCES += res/resources.qrc
