TARGET = osrm
TEMPLATE = lib
CONFIG += staticlib

ROOT_DIR = ../..

!win32* : QMAKE_CXXFLAGS *= -Wno-unused-function -Wno-unused-const-variable -Wno-ignored-qualifiers

include($$ROOT_DIR/common.pri)

DEFINES *= BOOST_ERROR_CODE_HEADER_ONLY
INCLUDEPATH *= osrm-backend/Include

SOURCES += \
    osrm-backend/Algorithms/DouglasPeucker.cpp \
    osrm-backend/Algorithms/PolylineCompressor.cpp \
#    Contractor/EdgeBasedGraphFactory.cpp \
#    Contractor/GeometryCompressor.cpp \
#    Contractor/TemporaryStorage.cpp \
#    datastore.cpp \
    osrm-backend/DataStructures/Coordinate.cpp \
#    DataStructures/HilbertValue.cpp \
#    DataStructures/ImportEdge.cpp \
#    DataStructures/ImportNode.cpp \
#    DataStructures/RestrictionMap.cpp \
    osrm-backend/DataStructures/RouteParameters.cpp \
    osrm-backend/DataStructures/SearchEngineData.cpp \
    osrm-backend/Descriptors/DescriptionFactory.cpp \
#    osrm-backend/Util/FingerPrint.cpp \
#    Extractor/BaseParser.cpp \
#    Extractor/ExtractionContainers.cpp \
#    Extractor/ExtractorCallbacks.cpp \
#    Extractor/PBFParser.cpp \
#    Extractor/ScriptingEnvironment.cpp \
#    Extractor/XMLParser.cpp \
#    extractor.cpp \
#    Library/OSRM_impl.cpp \
#    prepare.cpp \
#    routed.cpp \
#    Server/Connection.cpp \
#    Server/Http/Reply.cpp \
#    Server/RequestHandler.cpp \
#    Server/RequestParser.cpp \
#    Tools/components.cpp \
#    Tools/io-benchmark.cpp \
#    Tools/simpleclient.cpp \
#    Tools/unlock_all_mutexes.cpp \
    ../boost/libs/iostreams/src/mapped_file.cpp \
    ../boost/libs/filesystem/src/operations.cpp \
    ../boost/libs/filesystem/src/path.cpp \
    boost_stub.cpp \

HEADERS += \
    osrm-backend/osrm/Include/Coordinate.h \
    osrm-backend/DataStructures/SearchEngineData.h \
    osrm-backend/DataStructures/RouteParameters.h \
    osrm-backend/Algorithms/DouglasPeucker.h \
    osrm-backend/Descriptors/DescriptionFactory.h \
    osrm-backend/Util/FingerPrint.h \
