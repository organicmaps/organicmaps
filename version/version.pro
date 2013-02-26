# Used to generate project version header

TARGET = version
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ..

include($$ROOT_DIR/common.pri)

# Generate version header
VERSION_HEADER = ../../omim/version/version.hpp

versiontarget.target = $$VERSION_HEADER
win32 {
  versiontarget.commands = bash.exe $${IN_PWD}/../tools/unix/generate_version.sh $${IN_PWD}/.. $$VERSION_MAJOR $$VERSION_MINOR $$VERSION_HEADER
}
!win32 {
  versiontarget.commands = $${IN_PWD}/../tools/unix/generate_version.sh $${IN_PWD}/.. $$VERSION_MAJOR $$VERSION_MINOR $$VERSION_HEADER
}
versiontarget.depends = FORCE

PRE_TARGETDEPS += $$VERSION_HEADER
# regenerate version only in release and production builds
CONFIG(production) {
  QMAKE_EXTRA_TARGETS += versiontarget
}
# also regenerate if file doesn't exist
!exists( $$VERSION_HEADER ) {
  QMAKE_EXTRA_TARGETS += versiontarget
}

HEADERS += \
  version.hpp \
  ver_serialization.hpp \

SOURCES += \
  empty_stub.cpp \
