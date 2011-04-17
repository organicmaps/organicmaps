# This file is a template commmon for all qmake projects.
#
# To use it, define ROOT_DIR variable and include($$ROOT_DIR/common.pri)

# our own version variables
VERSION_MAJOR = 1
VERSION_MINOR = 0

# qt's variable
VERSION = $${VERSION_MAJOR}.$${VERSION_MINOR}

# Additional include directories, common to most projects.
INCLUDEPATH += $$ROOT_DIR/3party/boost

CONFIG(release, debug|release) {
  DEFINES += RELEASE _RELEASE
  CONFIG_NAME = release
} else {
  DEFINES += DEBUG _DEBUG
  CONFIG_NAME = debug
}

CONFIG(production) {
  DEFINES += OMIM_PRODUCTION
} else {
  # enable debugging information for non-production release builds
  unix|win32-g++ {
    QMAKE_CFLAGS_RELEASE += -g
    QMAKE_CXXFLAGS_RELEASE += -g
  }
}

BINARIES_PATH = $$ROOT_DIR/out/$$CONFIG_NAME
TEMP_PATH = $$ROOT_DIR/out/$$CONFIG_NAME/tmp/$$TARGET

DESTDIR = $$BINARIES_PATH
OBJECTS_DIR = $$TEMP_PATH
RCC_DIR = $$TEMP_PATH
MOC_DIR = $$TEMP_PATH
UI_DIR = $$TEMP_PATH

LIBS += -L$$BINARIES_PATH

# By default, do not include base QT classes in any project.
QT -= core gui

# Turn on compiler warnings.
CONFIG += warn_on

# Libraries' dependencies hack.
win32-msvc2008|symbian {
  LIB_EXT = .lib
  LIB_PREFIX =
}
unix|win32-g++|bada-simulator {
 LIB_EXT = .a
 LIB_PREFIX = lib
}

# Add libraries' dependencies.
for(project, DEPENDENCIES) {
  PRE_TARGETDEPS += $$BINARIES_PATH/$$LIB_PREFIX$$project$$LIB_EXT
  LIBS += -l$$project
}

INCLUDEPATH += $$ROOT_DIR/3party/protobuf/src/

# Windows-specific options for all projects
win32-msvc2008 {
  QMAKE_CLEAN += *.user
  DEFINES += _SCL_SECURE_NO_WARNINGS _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS NOMINMAX NO_MIN_MAX
  QMAKE_CXXFLAGS += -wd4100 -Zi
  QMAKE_CXXFLAGS += /Fd$${DESTDIR}/$${TARGET}.pdb
  QMAKE_CFLAGS += /Fd$${DESTDIR}/$${TARGET}.pdb
  QMAKE_LFLAGS += /PDB:$${DESTDIR}/$${TARGET}.pdb /DEBUG

  QMAKE_CXXFLAGS_RELEASE -= -O2
  # don't set -GL - bug in msvc
  QMAKE_CXXFLAGS_RELEASE += -Ox -Ob2 -Oi -Ot
  # don't set /LTCG - bug in msvc
  QMAKE_LFLAGS_RELEASE += /MACHINE:X86
 }

unix|win32-g++ {
  QMAKE_CXXFLAGS_WARN_ON += -Wno-sign-compare -Wno-strict-aliasing
  # experimental
  QMAKE_CFLAGS_RELEASE -= -O2
  QMAKE_CFLAGS_RELEASE += -O3
  QMAKE_CXXFLAGS_RELEASE -= -O2
  QMAKE_CXXFLAGS_RELEASE += -O3  
  QMAKE_CFLAGS_RELEASE *= -ffast-math
  QMAKE_CXXFLAGS_RELEASE *= -ffast-math
}

linux-g++-64 {
  QMAKE_CC                = gcc-4.5
  QMAKE_CXX               = g++-4.5
  QMAKE_CFLAGS_RELEASE   += -flto
  QMAKE_CXXFLAGS_RELEASE += -flto
  QMAKE_LFLAGS_RELEASE   += -flto
}

win32-g++ {
  QMAKE_CFLAGS *= -Wextra
  QMAKE_CXXFLAGS *= -Wextra
}

macx-g++ {
  # minimum supported Mac OS X version
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5
  QMAKE_CFLAGS *= -mmacosx-version-min=10.5
  QMAKE_CXXFLAGS *= -mmacosx-version-min=10.5
}
