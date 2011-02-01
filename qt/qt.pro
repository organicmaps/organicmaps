# Main application in qt.
ROOT_DIR = ..
DEPENDENCIES = map storage indexer yg platform geometry coding base freetype expat fribidi version

include($$ROOT_DIR/common.pri)

TARGET = MapsWithMe
TEMPLATE = app

QT *= core gui opengl network

win32 {
  LIBS += -lopengl32 -lws2_32 -lshell32
  RC_FILE = res/windows.rc
}

macx {
  ICON = res/mac.icns
  PLIST_FILE = Info.plist
  # path to original plist, which will be processed by qmake and later by us
  QMAKE_INFO_PLIST = res/$${PLIST_FILE}

  # fix version directly in bundle's Info.plist
  PLIST_PATH = $${DESTDIR}/$${TARGET}.app/Contents/$${PLIST_FILE}
  QMAKE_POST_LINK = $${IN_PWD}/../tools/unix/process_plist.sh $${IN_PWD}/.. $$VERSION_MAJOR $$VERSION_MINOR $$PLIST_PATH

  # Bundle Resouces
  CLASSIFICATOR_RESOURCES.files = ../data/classificator.txt ../data/drawing_rules.bin ../data/visibility.txt
  CLASSIFICATOR_RESOURCES.path = Contents/Resources
  SKIN_RESOURCES.files = ../data/basic.skn ../data/basic_highres.skn ../data/symbols_48.png ../data/symbols_24.png
  SKIN_RESOURCES.path = Contents/Resources
  FONT_RESOURCES.files = ../data/01_dejavusans.ttf \
                         ../data/02_wqy-microhei.ttf \
                         ../data/03_jomolhari-id-a3d.ttf \
                         ../data/04_padauk.ttf \
                         ../data/05_khmeros.ttf \
                         ../data/06_umpush.ttf \
                         ../data/07_abyssinica_sil_r.ttf \
                         ../data/08_lohit_as.ttf \
                         ../data/09_lohit_bn.ttf \
                         ../data/10_lohit_gu.ttf \
                         ../data/11_lohit_hi.ttf \
                         ../data/12_lohit_kn.ttf \
                         ../data/13_lohit_kok.ttf \
                         ../data/14_lohit_ks.ttf \
                         ../data/15_lohit_mai.ttf \
                         ../data/16_lohit_ml.ttf \
                         ../data/17_lohit_mr.ttf \
                         ../data/18_lohit_ne.ttf \
                         ../data/19_lohit_or.ttf \
                         ../data/20_lohit_pa.ttf \
                         ../data/21_lohit_sd.ttf \
                         ../data/22_lohit_ta.ttf \
                         ../data/23_lohit_te.ttf

  FONT_RESOURCES.path = Contents/Resources
  QMAKE_BUNDLE_DATA += CLASSIFICATOR_RESOURCES SKIN_RESOURCES FONT_RESOURCES
}

win32-g++ {
  LIBS += -lpthread
}

SOURCES += \
  main.cpp \
  mainwindow.cpp \
  searchwindow.cpp \
  widgets.cpp \
  update_dialog.cpp \
  draw_widget.cpp \
  classificator_tree.cpp \
  proxystyle.cpp \
  slider_ctrl.cpp \
  about.cpp

HEADERS += \
  mainwindow.hpp \
  searchwindow.hpp \
  widgets.hpp \
  update_dialog.hpp \
  draw_widget.hpp \
  qt_window_handle.hpp \
  classificator_tree.hpp \
  proxystyle.hpp \
  slider_ctrl.hpp \
  about.hpp \

RESOURCES += res/resources.qrc \
