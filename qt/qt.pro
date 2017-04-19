# Main application in qt.
ROOT_DIR = ..

DEPENDENCIES = qt_common map drape_frontend openlr routing search storage tracking traffic routing_common \
               indexer drape partners_api local_ads platform editor geometry \
               coding base freetype expat jansson protobuf osrm stats_client \
               minizip succinct pugixml oauthcpp stb_image sdf_image icu

DEPENDENCIES += opening_hours \

include($$ROOT_DIR/common.pri)

TARGET = MAPS.ME
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

  ICON = res/mac.icns
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
                  ../data/countries.txt \
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
MDPI_SKIN_RES.path = $$DATADIR/resources-mdpi_clear
MDPI_SKIN_RES.files = ../data/resources-mdpi_clear/symbols.sdf ../data/resources-mdpi_clear/symbols.png
XHDPI_SKIN_RES.path = $$DATADIR/resources-xhdpi_clear
XHDPI_SKIN_RES.files = ../data/resources-xhdpi_clear/symbols.sdf ../data/resources-xhdpi_clear/symbols.png

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
#ALL_RESOURCES += DEFAULT_SKIN_RES

linux* {
  INSTALLS += $$ALL_RESOURCES
}

macx-* {
  QMAKE_BUNDLE_DATA += $$ALL_RESOURCES
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
    traffic_mode.cpp \
    traffic_panel.cpp \
    trafficmodeinitdlg.cpp \
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
    traffic_mode.hpp \
    traffic_panel.hpp \
    trafficmodeinitdlg.h \
    update_dialog.hpp \

RESOURCES += res/resources.qrc

FORMS += \
    trafficmodeinitdlg.ui
