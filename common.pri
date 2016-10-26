# This file is a template commmon for all qmake projects.
#
# To use it, define ROOT_DIR variable and include($$ROOT_DIR/common.pri)

# We use some library features that were introduced in Mac OS X 10.8.
# Qt5.6.x sets target OS X version to 10.7 which leads to compile errors.
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9

CONFIG(map_designer) {
  DEFINES *= STANDALONE_APP
  DEFINES *= BUILD_DESIGNER
}

# our own version variables
VERSION_MAJOR = 2
VERSION_MINOR = 4

# qt's variable
VERSION = $${VERSION_MAJOR}.$${VERSION_MINOR}

# ROOT_DIR is in INCLUDEPATH now according to the new include policy.
INCLUDEPATH *= $$ROOT_DIR
# Do not set DEPENDPATH - it causes rebuild all with minor changes.
#DEPENDPATH  *= $$ROOT_DIR
INCLUDEPATH *= $$ROOT_DIR/3party/boost
INCLUDEPATH *= $$ROOT_DIR/3party/glm

# Hack for Qt5 qmake to make it much faster.
# Turns off any external headers modification check (see DEPENDPATH).
# Do not remove depend_includepath in CONFIG. Need to check modifications within ROOT_DIR.
#CONFIG -= depend_includepath

CONFIG *= c++11

# Automatically enable release config for production
CONFIG(production) {
  CONFIG *= release
}

CONFIG(release, debug|release) {
  DEFINES *= RELEASE _RELEASE NDEBUG
  CONFIG(production) {
    CONFIG_NAME = production
  } else {
    CONFIG_NAME = release
  }
} else {
  DEFINES *= DEBUG _DEBUG
  CONFIG_NAME = debug
}

BINARIES_PATH = $$ROOT_DIR/out/$$CONFIG_NAME
TEMP_PATH = $$ROOT_DIR/out/$$CONFIG_NAME/tmp/$$TARGET

DESTDIR = $$BINARIES_PATH
OBJECTS_DIR = $$TEMP_PATH
RCC_DIR = $$TEMP_PATH
MOC_DIR = $$TEMP_PATH
UI_DIR = $$TEMP_PATH

QMAKE_LIBDIR = $$BINARIES_PATH $$QMAKE_LIBDIR

# By default, do not include base QT classes in any project.
QT -= core gui

# Libraries' dependencies hack.
win32-msvc* {
  LIB_EXT = .lib
  LIB_PREFIX =
}
unix|win32-g++|tizen* {
 LIB_EXT = .a
 LIB_PREFIX = lib
}

# Add libraries' dependencies.
for(project, DEPENDENCIES) {
  PRE_TARGETDEPS += $$BINARIES_PATH/$$LIB_PREFIX$$project$$LIB_EXT
  LIBS += -l$$project
}

#INCLUDEPATH += $$ROOT_DIR/3party/protobuf/src/

# Windows-specific options for all projects
win32 {
  DEFINES += _WIN32_WINNT=0x0501
  DEFINES += WINVER=0x0501
  DEFINES += _WIN32_IE=0x0501
  DEFINES += WIN32_LEAN_AND_MEAN=1
  DEFINES += NTDDI_VERSION=0x05010000
  LIBS *= -lShell32
}

win32-msvc* {
  QMAKE_CLEAN += *.user
  DEFINES += _SCL_SECURE_NO_WARNINGS _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS NOMINMAX NO_MIN_MAX BOOST_ALL_NO_LIB
  QMAKE_CXXFLAGS += /Zi /fp:fast
  QMAKE_CFLAGS += /Zi /fp:fast

  QMAKE_CXXFLAGS_RELEASE -= /O2
  # don't set -GL - bug in msvc2008
  QMAKE_CXXFLAGS_RELEASE += /Ox /GF
  # don't set /LTCG - bug in msvc2008
  QMAKE_LFLAGS_RELEASE += /MACHINE:X86

  QMAKE_LFLAGS *= /OPT:REF
  QMAKE_LFLAGS_RELEASE *= /OPT:REF,ICF
  QMAKE_LFLAGS_DEBUG *= /OPT:NOICF

  CONFIG(release, debug|release) {
    DEFINES += _SECURE_SCL=0
  }
}

win32-msvc201* {
  # disable tr1 and c++0x features to avoid build errors
#  DEFINES += _HAS_CPP0X=0
#  DEFINES += BOOST_NO_CXX11_HDR_ARRAY BOOST_NO_CXX11_HDR_TYPEINDEX BOOST_NO_CXX11_SMART_PTR
  QMAKE_CFLAGS *= /wd4100
  QMAKE_CXXFLAGS *= /wd4100
  QMAKE_CFLAGS_RELEASE += /GL
  QMAKE_CXXFLAGS_RELEASE += /GL
  QMAKE_LFLAGS_RELEASE += /LTCG
  QMAKE_LIB += /LTCG
}

# unix also works for Android
unix|win32-g++ {
  LIBS *= -lz
  QMAKE_CXXFLAGS_WARN_ON += -Wno-sign-compare -Wno-strict-aliasing -Wno-unused-parameter

  # -Wno-unused-local-typedef is not supported on clang 3.5.
  IS_CLANG35 = $$system( echo | $$QMAKE_CXX -dM -E - | grep '__clang_version__.*3\.5.*' )
  if (isEmpty(IS_CLANG35)){
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-local-typedef
  }
  # TODO: Check if we really need these warnings on every platform (by syershov).
  *-clang* {
    QMAKE_CXXFLAGS_WARN_ON += -Wno-sign-conversion -Werror=return-type
  }

tizen{
  QMAKE_CFLAGS_RELEASE -= -O2
  QMAKE_CFLAGS_RELEASE += -O1
  QMAKE_CXXFLAGS_RELEASE -= -O2
  QMAKE_CXXFLAGS_RELEASE += -O1
  QMAKE_CFLAGS_RELEASE *= -ffast-math
  QMAKE_CXXFLAGS_RELEASE *= -ffast-math
} else {
  QMAKE_CFLAGS_RELEASE -= -O2
  QMAKE_CFLAGS_RELEASE += -O3
  QMAKE_CXXFLAGS_RELEASE -= -O2
  QMAKE_CXXFLAGS_RELEASE += -O3
  QMAKE_CFLAGS_RELEASE *= -ffast-math
  QMAKE_CXXFLAGS_RELEASE *= -ffast-math
}

}

linux-* {
  QMAKE_CFLAGS *= -fdata-sections -ffunction-sections
  QMAKE_CXXFLAGS *= -fdata-sections -ffunction-sections
  QMAKE_LFLAGS *= -Wl,--gc-sections -Wl,-Bsymbolic-functions
  linux-g++* {
    QMAKE_CFLAGS_RELEASE *= -ffloat-store
    QMAKE_CXXFLAGS_RELEASE *= -ffloat-store
  }
  # debian build requirements
  CONFIG(production) {
    QMAKE_CFLAGS_RELEASE = -g -O2 -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security
    QMAKE_CXXFLAGS_RELEASE = -D_FORTIFY_SOURCE=2 -g -O2 -fstack-protector --param=ssp-buffer-size=4 -Wformat -Werror=format-security
    QMAKE_LFLAGS *= -Wl,-z,relro
  }

}

android-g++ {
  QMAKE_CFLAGS *= -fdata-sections -ffunction-sections
  QMAKE_CXXFLAGS *= -fdata-sections -ffunction-sections
  QMAKE_LFLAGS *= -Wl,--gc-sections
}

win32-g++ {
  QMAKE_CFLAGS *= -Wextra
  QMAKE_CXXFLAGS *= -Wextra
  QMAKE_LFLAGS *= -s
  QMAKE_LFLAGS_RELEASE *= -O
  LIBS *= -lpthread
}

macx-* {
  QMAKE_LFLAGS *= -dead_strip
  LIBS *= "-framework Foundation" "-framework CFNetwork"

#  macx-clang {
#    QMAKE_CFLAGS_RELEASE -= -O3
#    QMAKE_CFLAGS_RELEASE += -O4 -flto
#    QMAKE_CXXFLAGS_RELEASE -= -O3
#    QMAKE_CXXFLAGS_RELEASE += -O4 -flto
#    QMAKE_LFLAGS_RELEASE += -O4 -flto
#  }
}

CONFIG(production) {
  DEFINES += OMIM_PRODUCTION
} else {
  # enable debugging information for non-production release builds
  unix|win32-g++ {
    QMAKE_CFLAGS_RELEASE += -g
    QMAKE_CXXFLAGS_RELEASE += -g
  } win32-msvc* {
    QMAKE_LFLAGS += /PDB:$${DESTDIR}/$${TARGET}.pdb /DEBUG
  }
}

# Fix boost compilation error
win32* {
    QMAKE_MOC = $$[QT_INSTALL_BINS]/moc.exe -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED
} else {
    QMAKE_MOC = $$[QT_INSTALL_BINS]/moc -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED
}

# Include with private variables.
HEADERS += \
  $$ROOT_DIR/private.h

# Standard includes wrappers.
HEADERS += \
  $$ROOT_DIR/std/algorithm.hpp \
  $$ROOT_DIR/std/array.hpp \
  $$ROOT_DIR/std/atomic.hpp \
  $$ROOT_DIR/std/auto_ptr.hpp \
  $$ROOT_DIR/std/bind.hpp \
  $$ROOT_DIR/std/bitset.hpp \
  $$ROOT_DIR/std/cctype.hpp \
  $$ROOT_DIR/std/cerrno.hpp \
  $$ROOT_DIR/std/chrono.hpp \
  $$ROOT_DIR/std/cmath.hpp \
  $$ROOT_DIR/std/complex.hpp \
  $$ROOT_DIR/std/condition_variable.hpp \
  $$ROOT_DIR/std/cstdarg.hpp \
  $$ROOT_DIR/std/cstdint.hpp \
  $$ROOT_DIR/std/cstdio.hpp \
  $$ROOT_DIR/std/cstdlib.hpp \
  $$ROOT_DIR/std/cstring.hpp \
  $$ROOT_DIR/std/ctime.hpp \
  $$ROOT_DIR/std/deque.hpp \
  $$ROOT_DIR/std/errno.hpp \
  $$ROOT_DIR/std/exception.hpp \
  $$ROOT_DIR/std/fstream.hpp \
  $$ROOT_DIR/std/function.hpp \
  $$ROOT_DIR/std/functional.hpp \
  $$ROOT_DIR/std/future.hpp \
  $$ROOT_DIR/std/initializer_list.hpp \
  $$ROOT_DIR/std/iomanip.hpp \
  $$ROOT_DIR/std/ios.hpp \
  $$ROOT_DIR/std/iostream.hpp \
  $$ROOT_DIR/std/iterator.hpp \
  $$ROOT_DIR/std/iterator_facade.hpp \
  $$ROOT_DIR/std/limits.hpp \
  $$ROOT_DIR/std/list.hpp \
  $$ROOT_DIR/std/map.hpp \
  $$ROOT_DIR/std/msvc_cpp11_workarounds.hpp \
  $$ROOT_DIR/std/mutex.hpp \
  $$ROOT_DIR/std/noncopyable.hpp \
  $$ROOT_DIR/std/numeric.hpp \
  $$ROOT_DIR/std/queue.hpp \
  $$ROOT_DIR/std/set.hpp \
  $$ROOT_DIR/std/shared_array.hpp \
  $$ROOT_DIR/std/shared_ptr.hpp \
  $$ROOT_DIR/std/sstream.hpp \
  $$ROOT_DIR/std/stack.hpp \
  $$ROOT_DIR/std/string.hpp \
  $$ROOT_DIR/std/systime.hpp \
  $$ROOT_DIR/std/target_os.hpp \
  $$ROOT_DIR/std/thread.hpp \
  $$ROOT_DIR/std/transform_iterator.hpp \
  $$ROOT_DIR/std/tuple.hpp \
  $$ROOT_DIR/std/type_traits.hpp \
  $$ROOT_DIR/std/typeinfo.hpp \
  $$ROOT_DIR/std/unique_ptr.hpp \
  $$ROOT_DIR/std/unordered_map.hpp \
  $$ROOT_DIR/std/unordered_set.hpp \
  $$ROOT_DIR/std/utility.hpp \
  $$ROOT_DIR/std/vector.hpp \
  $$ROOT_DIR/std/weak_ptr.hpp \
  $$ROOT_DIR/std/windows.hpp \
