# Project to compile generator tool on machines without OpenGL

QT_VERSION = $$[QT_VERSION]
QT_VERSION = $$split(QT_VERSION, ".")
QT_VER_MAJ = $$member(QT_VERSION, 0)
QT_VER_MIN = $$member(QT_VERSION, 1)

greaterThan(QT_VER_MAJ, 4) {
  cache()
}

TEMPLATE = subdirs
CONFIG += ordered

HEADERS += defines.hpp

SUBDIRS = 3party \
          base \
          coding \
          geometry \
          stats \
          indexer \
          platform \
          routing \
          storage \
          generator \
          generator/generator_tool \
