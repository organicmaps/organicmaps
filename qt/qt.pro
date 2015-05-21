# Main application in qt.
ROOT_DIR = ..
DEPENDENCIES = map render gui routing search storage indexer graphics platform anim geometry coding base \
               bzip2 freetype expat fribidi tomcrypt jansson protobuf osrm stats_client zlib succinct

drape {
  DEPENDENCIES *= drape_frontend drape
}

include($$ROOT_DIR/common.pri)

TARGET = MAPS.ME
TEMPLATE = app
CONFIG += warn_on
QT *= core widgets gui opengl

win32* {
  LIBS += -lopengl32 -lws2_32 -lshell32 -liphlpapi
  RC_FILE = res/windows.rc
  win32-msvc*: LIBS += -lwlanapi
  win32-g++: LIBS += -lpthread
}

win32*|linux* {
  QT *= network
}

linux* {
  isEmpty(PREFIX):PREFIX = /usr
  DEFINES += INSTALL_PREFIX=$$(PREFIX)
  BINDIR = $$PREFIX/bin

  DATADIR = $$PREFIX/share/mapsme
  RESDIR =  $$DATADIR

  FONTSDIR = /usr/share/fonts/truetype/mapsme/

  target.path = $$BINDIR
  desktop.path = /usr/share/applications/
  desktop.files += res/$${TARGET}.desktop

  OTHER_RES.path = $$RESDIR
  OTHER_RES.files = ../data/copyright.html ../data/eula.html ../data/welcome.html \
                    ../data/countries.txt \
                    ../data/languages.txt ../data/categories.txt \
                    ../data/packed_polygons.bin res/logo.png
  CLASSIFICATOR_RES.path = $$RESDIR
  CLASSIFICATOR_RES.files = ../data/classificator.txt \
                            ../data/types.txt \
                            ../data/drules_proto.bin
  MDPI_SKIN_RES.path = $$RESDIR/resources-mdpi
  MDPI_SKIN_RES.files = ../data/resources-mdpi/basic.skn ../data/resources-mdpi/symbols.png
  XHDPI_SKIN_RES.path = $$RESDIR/resources-xhdpi
  XHDPI_SKIN_RES.files = ../data/resources-xhdpi/basic.skn ../data/resources-xhdpi/symbols.png

  FONT_RES.path = $$FONTSDIR
  FONT_RES.files = ../data/00_roboto_regular.ttf \
                   ../data/01_dejavusans.ttf \
                   ../data/02_wqy-microhei.ttf \
                   ../data/03_jomolhari-id-a3d.ttf \
                   ../data/04_padauk.ttf \
                   ../data/05_khmeros.ttf \
                   ../data/06_code2000.ttf

  OTHER_RES.files += ../data/fonts_blacklist.txt \
                     ../data/fonts_whitelist.txt \
                     ../data/unicode_blocks.txt

  MWM_RES.path = $$RESDIR
  MWM_RES.files = ../data/World.mwm ../data/WorldCoasts.mwm

  INSTALLS += target desktop OTHER_RES CLASSIFICATOR_RES MDPI_SKIN_RES XHDPI_SKIN_RES FONT_RES MWM_RES
}

macx-* {
  LIBS *= "-framework CoreLocation" "-framework Foundation" "-framework CoreWLAN" \
          "-framework QuartzCore" "-framework IOKit"

  ICON = res/mac.icns
  PLIST_FILE = Info.plist
  # path to original plist, which will be processed by qmake and later by us
  QMAKE_INFO_PLIST = res/$${PLIST_FILE}

  # fix version directly in bundle's Info.plist
  PLIST_PATH = $${DESTDIR}/$${TARGET}.app/Contents/$${PLIST_FILE}
  QMAKE_POST_LINK = $${IN_PWD}/../tools/unix/process_plist.sh $${IN_PWD}/.. $$VERSION_MAJOR $$VERSION_MINOR $$PLIST_PATH


  # Bundle Resouces
  OTHER_RES.path = Contents/Resources
  OTHER_RES.files = ../data/copyright.html ../data/eula.html ../data/welcome.html \
                    ../data/countries.txt  \
                    ../data/languages.txt ../data/categories.txt \
                    ../data/packed_polygons.bin
  CLASSIFICATOR_RES.path = Contents/Resources
  CLASSIFICATOR_RES.files = ../data/classificator.txt \
                            ../data/types.txt \
                            ../data/drules_proto.bin
  SKIN_RES.path = Contents/Resources
  SKIN_RES.files = ../data/resources-mdpi/basic.skn ../data/resources-mdpi/symbols.png
  FONT_RES.path = Contents/Resources
  FONT_RES.files = ../data/01_dejavusans.ttf \
                   ../data/02_wqy-microhei.ttf \
                   ../data/03_jomolhari-id-a3d.ttf \
                   ../data/04_padauk.ttf \
                   ../data/05_khmeros.ttf \
                   ../data/06_code2000.ttf \
                   ../data/fonts_blacklist.txt \
                   ../data/fonts_whitelist.txt \
                   ../data/unicode_blocks.txt
  MWM_RES.path = Contents/Resources
  MWM_RES.files = ../data/World.mwm ../data/WorldCoasts.mwm

  QMAKE_BUNDLE_DATA += OTHER_RES CLASSIFICATOR_RES SKIN_RES FONT_RES MWM_RES
}

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    draw_widget.cpp \
    proxystyle.cpp \
    slider_ctrl.cpp \
    about.cpp \
    info_dialog.cpp \
    preferences_dialog.cpp \
    search_panel.cpp \
    update_dialog.cpp \

CONFIG(drape){
  SOURCES += \
    drape_surface.cpp \
    qtoglcontext.cpp \
    qtoglcontextfactory.cpp \
}

HEADERS += \
    mainwindow.hpp \
    widgets.hpp \
    draw_widget.hpp \
    proxystyle.hpp \
    slider_ctrl.hpp \
    about.hpp \
    info_dialog.hpp \
    preferences_dialog.hpp \
    search_panel.hpp \
    update_dialog.hpp \

CONFIG(drape){
  HEADERS += \
    drape_surface.hpp \
    qtoglcontext.hpp \
    qtoglcontextfactory.hpp \
}

RESOURCES += res/resources.qrc
