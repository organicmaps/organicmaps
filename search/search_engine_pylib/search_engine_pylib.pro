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
    QMAKE_LFLAGS_PLUGIN += -bundle

    LIBRARY_PYTHON=/Library/Frameworks/Python.framework/Versions/2.7
    SYSTEM_LIBRARY_PYTHON=/System/Library/Frameworks/Python.framework/Versions/2.7

    exists($$LIBRARY_PYTHON) {
        INCLUDEPATH += $$LIBRARY_PYTHON/include/python2.7
        LIBS += -L$$LIBRARY_PYTHON/lib -lpython2.7
    } else:exists($$SYSTEM_LIBRARY_PYTHON) {
        INCLUDEPATH += $$SYSTEM_LIBRARY_PYTHON/include/python2.7
        LIBS += -L$$SYSTEM_LIBRARY_PYTHON/lib -lpython2.7
    } else {
        error("Can't find Python2.7")
    }

    LIBS *= "-framework IOKit"
    LIBS *= "-framework SystemConfiguration"

    LIBS *= -L/usr/local/opt/qt5/lib
    LIBS *= "-framework QtCore"
    LIBS *= "-framework QtNetwork"

    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib -lboost_python

    INCLUDEPATH += /usr/local/opt/qt5/include
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
