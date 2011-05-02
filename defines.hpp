#pragma once

#define DATA_FILE_EXTENSION ".mwm"
#define DATA_FILE_TAG "dat"
#define GEOMETRY_FILE_TAG "geom"
#define TRIANGLE_FILE_TAG "trg"
#define INDEX_FILE_TAG "idx"
#define HEADER_FILE_TAG "header"

#define COUNTRIES_FILE  "countries.txt"
#define DATA_UPDATE_FILE "maps.update"
#define BINARY_UPDATE_FILE "binary.update"

#ifdef OMIM_PRODUCTION
  #define UPDATE_BASE_URL "http://data.mapswithme.com/"
#else
  #define UPDATE_BASE_URL "http://svobodu404popugajam.mapswithme.com:34568/maps/"
#endif

#define WORLD_FILE_NAME "World"

#define SETTINGS_FILE_NAME "settings.ini"

#define DEFAULT_AUTO_UPDATES_ENABLED true
