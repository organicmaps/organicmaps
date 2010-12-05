# Main application in qt.
ROOT_DIR = ..
DEPENDENCIES = map indexer platform yg geometry coding base expat version

include($$ROOT_DIR/common.pri)

TARGET = MapsWithMe
TEMPLATE = app

QT *= core gui opengl network

win32 {
  LIBS += -lopengl32 -lws2_32
}

macx {
  ICON = res/mac.icns
  PLIST_FILE = Info.plist
  # path to original plist, which will be processed by qmake and later by us
  QMAKE_INFO_PLIST = res/$${PLIST_FILE}

  # fix version directly in bundle's Info.plist
  PLIST_PATH = $${DESTDIR}/$${TARGET}.app/Contents/$${PLIST_FILE}
  QMAKE_POST_LINK = $${IN_PWD}/../tools/unix/process_plist.sh $${IN_PWD}/.. $$VERSION_MAJOR $$VERSION_MINOR $$PLIST_PATH
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
