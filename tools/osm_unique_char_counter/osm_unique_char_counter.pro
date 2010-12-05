# -----------------------------------------------------
# Project created by Alex Zolotarev 2010-01-21T13:23:29
# -----------------------------------------------------
include(../../common.pro.include)

QT       -= gui
TARGET = osm_unique_char_counter
CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE = app

# Additional include directories
INCLUDEPATH *= ../../3party/expat/lib \
               ../../3party/boost

# Configure intermediate and output directories
CONFIG(release, debug|release) { 
    BINARIES_PATH = ../../out/release
    TEMP_PATH = ../../out/release/tmp/$$TARGET
}
else { 
    BINARIES_PATH = ../../out/debug
    TEMP_PATH = ../../out/debug/tmp/$$TARGET
}
DESTDIR = $$BINARIES_PATH
OBJECTS_DIR = $$TEMP_PATH
RCC_DIR = $$TEMP_PATH
MOC_DIR = $$TEMP_PATH
UI_DIR = $$TEMP_PATH

# Configure some specific compiler options
win32-msvc2008 { 
    QMAKE_CFLAGS_DEBUG += /Fd$${DESTDIR}/$${TARGET}.pdb
    QMAKE_CXXFLAGS_DEBUG += /Fd$${DESTDIR}/$${TARGET}.pdb
    QMAKE_LFLAGS += /PDB:$${DESTDIR}/$${TARGET}.pdb
}

# Configure library dependencies for all libraries in LIBS
PRE_TARGETDEPS = $$BINARIES_PATH/$${LIB_PREFIX}expat$$LIB_EXT

LIBS += -L$$BINARIES_PATH \
        -lexpat

HEADERS += ../../indexer/xmlparser.h
           
SOURCES += main.cpp
