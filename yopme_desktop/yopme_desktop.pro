#-------------------------------------------------
#
# Project created by QtCreator 2013-09-08T12:17:34
#
#-------------------------------------------------

ROOT_DIR = ..
DEPENDENCIES = map gui routing search storage indexer graphics platform anim geometry coding base \
               bzip2 freetype expat fribidi tomcrypt jansson protobuf zlib


include($$ROOT_DIR/common.pri)

QT       *= core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32*|linux* {
  QT *= network
}

TARGET = yopme_desktop
TEMPLATE = app

macx-* {
  LIBS *= "-framework CoreLocation" "-framework Foundation" "-framework CoreWLAN" \
          "-framework QuartzCore" "-framework IOKit"

  # Bundle Resouces
  OTHER_RES.path = Contents/Resources
  OTHER_RES.files = ../data/about.html \
                    ../data/eula.html \
                    ../data/welcome.html \
                    ../data/countries.txt  \
                    ../data/languages.txt \
                    ../data/categories.txt \
                    ../data/packed_polygons.bin
  CLASSIFICATOR_RES.path = Contents/Resources
  CLASSIFICATOR_RES.files = ../data/classificator.txt \
                            ../data/types.txt \
                            ../data/drules_proto.bin
  SKIN_RES.path = Contents/Resources/resources-mdpi
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

SOURCES += main.cpp \
           mainwindow.cpp \
           glwidget.cpp \

HEADERS  += mainwindow.hpp \
            glwidget.hpp \

FORMS    += mainwindow.ui
