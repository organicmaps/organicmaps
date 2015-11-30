TARGET = drape_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app
DEFINES += OGL_TEST_ENABLED GTEST_DONT_DEFINE_TEST COMPILER_TESTS

DEPENDENCIES = qt_tstfrm platform coding base gmock freetype fribidi expat tomcrypt
ROOT_DIR = ../..
SHADER_COMPILE_ARGS = $$PWD/../shaders shader_index.txt shader_def
include($$ROOT_DIR/common.pri)

QT *= core gui widgets

DRAPE_DIR = ..
include($$DRAPE_DIR/drape_common.pri)

INCLUDEPATH *= $$ROOT_DIR/3party/gmock/include $$ROOT_DIR/3party/gmock/gtest/include

macx-* : LIBS *= "-framework CoreLocation"

SOURCES += \
    attribute_provides_tests.cpp \
    batcher_tests.cpp \
    bingind_info_tests.cpp \
    buffer_tests.cpp \
    compile_shaders_test.cpp \
    failure_reporter.cpp \
    font_texture_tests.cpp \
    fribidi_tests.cpp \
    glfunctions.cpp \
    glmock_functions.cpp \
    glyph_mng_tests.cpp \
    glyph_packer_test.cpp \
    img.cpp \
    pointers_tests.cpp \
    stipple_pen_tests.cpp \
    testingmain.cpp \
    texture_of_colors_tests.cpp \
    uniform_value_tests.cpp \

HEADERS += \
    dummy_texture.hpp \
    glmock_functions.hpp \
    img.hpp \
    memory_comparer.hpp \
