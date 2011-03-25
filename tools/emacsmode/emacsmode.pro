QTC_SOURCE = ../../../qt-creator-2.2.0-beta-src
QTC_BUILD = ../../../qtcreator-build

TEMPLATE = lib
TARGET = EmacsMode
IDE_SOURCE_TREE = $$QTC_SOURCE
IDE_BUILD_TREE = $$QTC_BUILD
PROVIDER = Spliny
DESTDIR = $$QTC_BUILD/lib/qtcreator/plugins/Spliny
LIBS += -L$$QTC_BUILD/bin/Qt\ Creator.app/Contents/PlugIns/Nokia

include($$QTC_SOURCE/src/qtcreatorplugin.pri)
include($$QTC_SOURCE/src/libs/cplusplus/cplusplus.pri)
include($$QTC_SOURCE/src/plugins/projectexplorer/projectexplorer.pri)
include($$QTC_SOURCE/src/plugins/coreplugin/coreplugin.pri)
include($$QTC_SOURCE/src/plugins/texteditor/texteditor.pri)
include($$QTC_SOURCE/src/plugins/cppeditor/cppeditor.pri)
include($$QTC_SOURCE/src/plugins/find/find.pri)
include($$QTC_SOURCE/src/shared/indenter/indenter.pri)

# DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII
QT += gui
SOURCES += emacsmodehandler.cpp \
    emacsmodeplugin.cpp \
    emacsmodeactions.cpp \
    emacs_shortcut.cpp

HEADERS += emacsmodehandler.h \
    emacsmodeplugin.h \
    emacsmodeactions.h \
    emacs_shortcut.h

FORMS += emacsmodeoptions.ui
OTHER_FILES += EmacsMode.pluginspec

INCLUDEPATH *= ../../3party/boost
