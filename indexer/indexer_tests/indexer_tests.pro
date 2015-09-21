TARGET = indexer_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = indexer platform geometry coding base normalize protobuf tomcrypt
!linux {
DEPENDENCIES += opening_hours
}

include($$ROOT_DIR/common.pri)

QT *= core

!linux {
SOURCES += opening_hours_test.cpp
}

HEADERS += \
    test_mwm_set.hpp \
    test_polylines.hpp \

SOURCES += \
    ../../testing/testingmain.cpp \
    categories_test.cpp \
    cell_coverer_test.cpp \
    cell_id_test.cpp \
    checker_test.cpp \
    city_rank_table_test.cpp \
    features_offsets_table_test.cpp \
    geometry_coding_test.cpp \
    geometry_serialization_test.cpp \
    index_builder_test.cpp \
    index_test.cpp \
    interval_index_test.cpp \
    mercator_test.cpp \
    mwm_set_test.cpp \
    point_to_int64_test.cpp \
    scales_test.cpp \
    search_string_utils_test.cpp \
    sort_and_merge_intervals_test.cpp \
    test_polylines.cpp \
    test_type.cpp \
    visibility_test.cpp \
