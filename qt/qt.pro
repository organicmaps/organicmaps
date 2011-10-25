# Main application in qt.
ROOT_DIR = ..
DEPENDENCIES = words map search storage indexer yg platform geometry coding base \
               bzip2 freetype expat fribidi tomcrypt jansson version

include($$ROOT_DIR/common.pri)

TARGET = MapsWithMe
TEMPLATE = app

QT *= core gui opengl network

win32 {
  LIBS += -lopengl32 -lws2_32 -lshell32 -liphlpapi
  RC_FILE = res/windows.rc
  win32-msvc*: LIBS += -lwlanapi
  win32-g++: LIBS += -lpthread
}

macx* {
  LIBS *= "-framework CoreLocation" "-framework Foundation" "-framework CoreWLAN" \
          "-framework QuartzCore" "-framework IOKit"

  ICON = res/mac.icns
  PLIST_FILE = Info.plist
  # path to original plist, which will be processed by qmake and later by us
  QMAKE_INFO_PLIST = res/$${PLIST_FILE}

  # fix version directly in bundle's Info.plist
  PLIST_PATH = $${DESTDIR}/$${TARGET}.app/Contents/$${PLIST_FILE}
  QMAKE_POST_LINK = $${IN_PWD}/../tools/unix/process_plist.sh $${IN_PWD}/.. $$VERSION_MAJOR $$VERSION_MINOR $$PLIST_PATH

  CONFIG(production) {
    # Bundle Resouces
    OTHER_RES.path = Contents/Resources
    OTHER_RES.files = ../data/about-travelguide-desktop.html ../data/eula.html ../data/welcome.html \
                      ../data/countries.txt  \
                      ../data/dictionary.slf ../data/languages.txt ../data/categories.txt
    CLASSIFICATOR_RES.path = Contents/Resources
    CLASSIFICATOR_RES.files = ../data/classificator.txt ../data/drawing_rules.bin ../data/visibility.txt ../data/types.txt
    SKIN_RES.path = Contents/Resources
    SKIN_RES.files = ../data/basic.skn ../data/symbols_24.png
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
    MWM_RES.files = ../data/World.mwm

    QMAKE_BUNDLE_DATA += OTHER_RES CLASSIFICATOR_RES SKIN_RES FONT_RES MWM_RES
  }
}

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    widgets.cpp \
    draw_widget.cpp \
    proxystyle.cpp \
    slider_ctrl.cpp \
    about.cpp \
    info_dialog.cpp \
    preferences_dialog.cpp \
    search_panel.cpp \

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

RESOURCES += res/resources.qrc

# removed for desktop releases
!CONFIG(no_downloader) {
  QT *= webkit

  SOURCES += \
    update_dialog.cpp \
    classificator_tree.cpp \
    guide_page.cpp

  HEADERS += \
    update_dialog.hpp \
    classificator_tree.hpp \
    guide_page.hpp
}
