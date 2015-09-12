TARGET = drape_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app
DEFINES += OGL_TEST_ENABLED GTEST_DONT_DEFINE_TEST COMPILER_TESTS

DEPENDENCIES = qt_tstfrm platform coding base gmock freetype expat tomcrypt
ROOT_DIR = ../..
SHADER_COMPILE_ARGS = $$PWD/../shaders shader_index.txt shader_def
include($$ROOT_DIR/common.pri)

QT *= core gui widgets

DRAPE_DIR = ..
include($$DRAPE_DIR/drape_common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/gmock/include $$ROOT_DIR/3party/gmock/gtest/include $$ROOT_DIR/3party/expat/lib

macx-* : LIBS *= "-framework CoreLocation"

SOURCES += \
    glfunctions.cpp \
    testingmain.cpp \
    failure_reporter.cpp \
    glmock_functions.cpp \
    buffer_tests.cpp \
    uniform_value_tests.cpp \
    attribute_provides_tests.cpp \
    compile_shaders_test.cpp \
    batcher_tests.cpp \
    pointers_tests.cpp \
    bingind_info_tests.cpp \
    stipple_pen_tests.cpp \
    texture_of_colors_tests.cpp \
    glyph_mng_tests.cpp \
    glyph_packer_test.cpp \
    font_texture_tests.cpp \
    img.cpp \

HEADERS += \
    glmock_functions.hpp \
    memory_comparer.hpp \
    img.hpp \
    dummy_texture.hpp \
