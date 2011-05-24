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
win32-msvc*|symbian {
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

win32 {
  DEFINES += _WIN32_WINNT=0x0501
  DEFINES += WINVER=0x0501
  DEFINES += _WIN32_IE=0x0501
  DEFINES += WIN32_LEAN_AND_MEAN=1
  DEFINES += NTDDI_VERSION=0x05010000
}

# Windows-specific options for all projects
win32-msvc* {
  QMAKE_CLEAN += *.user
  DEFINES += _SCL_SECURE_NO_WARNINGS _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS NOMINMAX NO_MIN_MAX
  QMAKE_CXXFLAGS += /Fd$${DESTDIR}/$${TARGET}.pdb /Zi /fp:fast
  QMAKE_CFLAGS += /Fd$${DESTDIR}/$${TARGET}.pdb /Zi /fp:fast
  QMAKE_LFLAGS += /PDB:$${DESTDIR}/$${TARGET}.pdb

  QMAKE_CXXFLAGS_RELEASE -= /O2
  # don't set -GL - bug in msvc2008
  QMAKE_CXXFLAGS_RELEASE += /Ox
  # don't set /LTCG - bug in msvc2008
  QMAKE_LFLAGS_RELEASE += /MACHINE:X86 /OPT:REF

  CONFIG(release, debug|release) {
    DEFINES += _SECURE_SCL=0
  }
 }

win32-msvc2010 {
  DEFINES += _HAS_CPP0X=0 # disable tr1 and c++0x features to avoid build errors
  QMAKE_CFLAGS_RELEASE += /GL
  QMAKE_CXXFLAGS_RELEASE += /GL
  QMAKE_LFLAGS_RELEASE += /LTCG
  QMAKE_LIB += /LTCG
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
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
  QMAKE_CFLAGS *= -mmacosx-version-min=10.6
  QMAKE_CXXFLAGS *= -mmacosx-version-min=10.6
  # replace 10.5 with 10.6
  QMAKE_CFLAGS_X86_64 -= -mmacosx-version-min=10.5
  QMAKE_CXXFLAGS_X86_64 -= -mmacosx-version-min=10.5
  QMAKE_CFLAGS_X86_64 += -mmacosx-version-min=10.6    # CXX is made based on CFLAGS
  QMAKE_OBJECTIVE_CFLAGS_X86_64 -= -mmacosx-version-min=10.5
  QMAKE_OBJECTIVE_CFLAGS_X86_64 += -mmacosx-version-min=10.6
  QMAKE_LFLAGS_X86_64 -= -mmacosx-version-min=10.5
  QMAKE_LFLAGS_X86_64 += -mmacosx-version-min=10.6
}
