# Main application in qt.
ROOT_DIR = ..
DEPENDENCIES = words map storage search indexer yg platform geometry coding base \
               bzip2 freetype expat fribidi tomcrypt jansson version

include($$ROOT_DIR/common.pri)

TARGET = MapsWithMe
TEMPLATE = app

QT *= core gui opengl network

win32 {
  LIBS += -lopengl32 -lws2_32 -lshell32
  RC_FILE = res/windows.rc
}

macx {
  LIBS += -framework CoreLocation -framework Foundation -framework CoreWLAN

  ICON = res/mac.icns
  PLIST_FILE = Info.plist
  # path to original plist, which will be processed by qmake and later by us
  QMAKE_INFO_PLIST = res/$${PLIST_FILE}

  # fix version directly in bundle's Info.plist
  PLIST_PATH = $${DESTDIR}/$${TARGET}.app/Contents/$${PLIST_FILE}
  QMAKE_POST_LINK = $${IN_PWD}/../tools/unix/process_plist.sh $${IN_PWD}/.. $$VERSION_MAJOR $$VERSION_MINOR $$PLIST_PATH

  # Bundle Resouces
  # OTHER_RESOURCES.files = ../data/about-travelguide-desktop.html ../data/eula.html ../data/World.mwm
  OTHER_RESOURCES.path = Contents/Resources
  CLASSIFICATOR_RESOURCES.files = ../data/classificator.txt ../data/drawing_rules.bin ../data/visibility.txt
  CLASSIFICATOR_RESOURCES.path = Contents/Resources
  SKIN_RESOURCES.files = ../data/basic.skn ../data/basic_highres.skn ../data/symbols_48.png ../data/symbols_24.png
  SKIN_RESOURCES.path = Contents/Resources
  FONT_RESOURCES.files = ../data/01_dejavusans.ttf \
                         ../data/02_wqy-microhei.ttf \
                         ../data/03_jomolhari-id-a3d.ttf \
                         ../data/04_padauk.ttf \
                         ../data/05_khmeros.ttf \
                         ../data/06_code2000.ttf

  FONT_RESOURCES.path = Contents/Resources
  QMAKE_BUNDLE_DATA += OTHER_RESOURCES CLASSIFICATOR_RESOURCES SKIN_RESOURCES FONT_RESOURCES
}

win32-g++ {
  LIBS += -lpthread
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

HEADERS += \
    mainwindow.hpp \
    widgets.hpp \
    draw_widget.hpp \
    qt_window_handle.hpp \
    proxystyle.hpp \
    slider_ctrl.hpp \
    about.hpp \
    info_dialog.hpp \
    preferences_dialog.hpp \

RESOURCES += res/resources.qrc

# removed for desktop releases
CONFIG(debug, debug|release) {
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
