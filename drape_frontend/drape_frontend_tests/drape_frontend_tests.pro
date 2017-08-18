
TARGET = drape_frontend_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

DEFINES += COMPILER_TESTS

DEPENDENCIES = drape_frontend drape platform indexer geometry coding base expat stats_client stb_image sdf_image icu

SHADER_COMPILE_ARGS = $$PWD/../shaders shader_index.txt shaders_lib.glsl $$PWD shader_def_for_tests
CMDRES = $$system(python $$PWD/../../tools/autobuild/shader_preprocessor.py $$SHADER_COMPILE_ARGS)
!isEmpty($$CMDRES):message($$CMDRES)

DEL_SHADERS_COMPILERS = $$system(rm -rf $$PWD/../../data/shaders_compiler)
!isEmpty(DEL_SHADERS_COMPILERS):message(DEL_SHADERS_COMPILERS)

COPY_SHADERS_COMPILERS = $$system(cp -r $$PWD/../../tools/shaders_compiler $$PWD/../../data)
!isEmpty(COPY_SHADERS_COMPILERS):message(COPY_SHADERS_COMPILERS)

ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

QT *= opengl

macx-* {
  LIBS *= "-framework CoreLocation" "-framework Foundation" "-framework CoreWLAN" \
          "-framework QuartzCore" "-framework IOKit" "-framework Cocoa" "-framework SystemConfiguration"
}
win32*|linux* {
  QT *= network
}

SOURCES += \
  ../../testing/testingmain.cpp \
  compile_shaders_test.cpp \
  navigator_test.cpp \
  path_text_test.cpp \
  shader_def_for_tests.cpp \
  user_event_stream_tests.cpp \

HEADERS += \
  shader_def_for_tests.hpp \
