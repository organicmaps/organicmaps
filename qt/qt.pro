# Main application in qt.
ROOT_DIR = ..
DEPENDENCIES = map indexer platform yg geometry coding base expat version

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
  SKIN_RESOURCES.files = ../data/basic.skn ../data/symbols_24.png \
    ../data/dejavusans_8.png ../data/dejavusans_10.png ../data/dejavusans_12.png \
    ../data/dejavusans_14.png ../data/dejavusans_16.png ../data/dejavusans_20.png \
    ../data/dejavusans_24.png
  SKIN_RESOURCES.path = Contents/Resources
  QMAKE_BUNDLE_DATA += CLASSIFICATOR_RESOURCES SKIN_RESOURCES
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

HEADERS += \
  mainwindow.hpp \
  searchwindow.hpp \
  widgets.hpp \
  update_dialog.hpp \
  draw_widget.hpp \
  qt_window_handle.hpp \
  classificator_tree.hpp \
  proxystyle.hpp \

RESOURCES += res/resources.qrc \
