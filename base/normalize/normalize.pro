# Separate library to avoid Release builds clang compilation hang
# with Normalization code. TODO: Refactor it out when clang will be fixed.
# See https://code.google.com/p/android/issues/detail?id=173992 (bug was also filed to Apple).

TARGET = normalize
TEMPLATE = lib
CONFIG += staticlib warn_on

ROOT_DIR = ../..

include($$ROOT_DIR/common.pri)

# Override optimization option. Compilation hangs with -O2 and -O3.
*-clang {
  QMAKE_CXXFLAGS_RELEASE -= -O2 -O3
  QMAKE_CXXFLAGS_RELEASE += -Oz
}

SOURCES += \
    ../normalize_unicode.cpp \
