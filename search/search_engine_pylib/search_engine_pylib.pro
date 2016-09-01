TARGET = search_engine_pylib

TEMPLATE = lib
CONFIG += plugin no_plugin_name_prefix

QMAKE_LFLAGS_PLUGIN -= -dynamiclib
QMAKE_EXTENSION_SHLIB = so
QT *= core network

ROOT_DIR = ../..
include($$ROOT_DIR/common.pri)

DEFINES += BOOST_PYTHON_DYNAMIC_LIB BOOST_BIND_NO_PLACEHOLDERS

INCLUDEPATH -= $$ROOT_DIR/3party/boost

# For Mac boost is built with libc++, for Linux with libstdc++.
# We do not support search_engine_pylib for other combinations of
# OS and c++ standard library.
macx-clang {
    INCLUDEPATH += /System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7
    LIBS += -L/System/Library/Frameworks/Python.framework/Versions/2.7/lib -lpython2.7

    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib -lboost_python
} else:linux-clang {
    INCLUDEPATH += /usr/include
    LIBS += -L/usr/lib/x86_64-linux-gnu/ -lboost_python

    INCLUDEPATH += /usr/include/python2.7
    LIBS += -L/usr/lib -lpython2.7
} else {
    error("Can't build search_engine_pylib - wrong spec $$QMAKESPEC")
}

LIBS += -lsearch_tests_support \
        -lsearch \
        -lstorage \
        -lindexer \
        -leditor \
        -lgeometry \
        -lplatform \
        -lcoding \
        -lbase \
        -lopening_hours \
        -ltomcrypt \
        -lsuccinct \
        -lpugixml \
        -lprotobuf \
        -ljansson \
        -loauthcpp \
        -lstats_client

SOURCES += api.cpp \
