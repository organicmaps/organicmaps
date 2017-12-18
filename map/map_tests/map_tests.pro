# Map library tests.

TARGET = map_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

ROOT_DIR = ../..
DEPENDENCIES = search_tests_support generator_tests_support indexer_tests_support generator \
               map drape_frontend routing traffic routing_common transit search storage tracking drape \
               ugc indexer partners_api local_ads platform editor mwm_diff bsdiff geometry coding base \
               freetype expat protobuf jansson osrm stats_client minizip succinct pugixml \
               stats_client tess2 stb_image sdf_image icu agg

DEPENDENCIES *= opening_hours

include($$ROOT_DIR/common.pri)

DEFINES *= OMIM_UNIT_TEST_WITH_QT_EVENT_LOOP

QT *= core opengl

macx-* {
  QT *= gui widgets network # needed for QApplication with event loop, to test async events (downloader, etc.)
  LIBS *= "-framework IOKit" "-framework QuartzCore" "-framework SystemConfiguration"
}

win*|linux* {
  QT *= network
}

win32*: LIBS *= -lOpengl32
macx-*: LIBS *= "-framework IOKit" "-framework SystemConfiguration"

SOURCES += \
  ../../testing/testingmain.cpp \
  address_tests.cpp \
  booking_availability_cache_test.cpp \
  booking_filter_test.cpp \
  bookmarks_test.cpp \
  chart_generator_tests.cpp \
  feature_getters_tests.cpp \
  ge0_parser_tests.cpp \
  geourl_test.cpp \
  gps_track_collection_test.cpp \
  gps_track_storage_test.cpp \
  gps_track_test.cpp \
  kmz_unarchive_test.cpp \
  mwm_url_tests.cpp \
  search_api_tests.cpp \
  transliteration_test.cpp \

!linux* {
  SOURCES += working_time_tests.cpp \
}
