#pragma once

#define DATA_FILE_EXTENSION ".mwm"
#define DATA_FILE_TAG "dat"
#define GEOMETRY_FILE_TAG "geom"
#define TRIANGLE_FILE_TAG "trg"
#define INDEX_FILE_TAG "idx"
#define SEARCH_INDEX_FILE_TAG "sdx"
#define HEADER_FILE_TAG "header"
#define VERSION_FILE_TAG "version"

#define COUNTRIES_FILE  "countries.txt"

#define WORLD_FILE_NAME "World"
#define WORLD_COASTS_FILE_NAME "WorldCoasts"

#define SETTINGS_FILE_NAME "settings.ini"

#define SEARCH_CATEGORIES_FILE_NAME "categories.txt"

#define PACKED_POLYGONS_FILE "packed_polygons.bin"
#define PACKED_POLYGONS_INFO_TAG "info"

/// How many langs we're supporting on indexing stage
#define MAX_SUPPORTED_LANGUAGES 64

#ifdef OMIM_PRODUCTION
  #define URL_SERVERS_LIST "http://mapswithme-metaserver.appspot.com/server_data/active_servers"
  #define DEFAULT_SERVERS_JSON "[\"http://1st.default.server/\",\"http://2nd.default.server/\"]"
#else
  #define URL_SERVERS_LIST "http://mwm-dev.appspot.com/server_data/active_servers"
  #define DEFAULT_SERVERS_JSON "[\"http://svobodu404popugajam.mapswithme.com:34568/maps/\",\"http://svobodu404popugajam.mapswithme.com:34568/maps/\"]"
#endif
