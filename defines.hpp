#pragma once

// Only defines and constexprs are allowed in this file.

#define DATA_FILE_EXTENSION ".mwm"
#define DATA_FILE_EXTENSION_TMP ".mwm.tmp"
#define DIFF_FILE_EXTENSION ".mwmdiff"
#define DIFF_APPLYING_FILE_EXTENSION ".diff.applying"
#define FONT_FILE_EXTENSION ".ttf"
#define OSM2FEATURE_FILE_EXTENSION ".osm2ft"
#define EXTENSION_TMP ".tmp"
#define ADDR_FILE_EXTENSION ".addr"
#define RAW_GEOM_FILE_EXTENSION ".rawgeom"

#define NODES_FILE "nodes.dat"
#define WAYS_FILE "ways.dat"
#define RELATIONS_FILE "relations.dat"
#define TOWNS_FILE "towns.csv"
#define OFFSET_EXT ".offs"
#define ID2REL_EXT ".id2rel"

#define CENTERS_FILE_TAG "centers"
#define DATA_FILE_TAG "dat"
#define GEOMETRY_FILE_TAG "geom"
#define TRIANGLE_FILE_TAG "trg"
#define INDEX_FILE_TAG "idx"
#define SEARCH_INDEX_FILE_TAG "sdx"
#define SEARCH_ADDRESS_FILE_TAG "addr"
#define CITIES_BOUNDARIES_FILE_TAG "cities_boundaries"
#define HEADER_FILE_TAG "header"
#define VERSION_FILE_TAG "version"
#define METADATA_FILE_TAG "meta"
#define METADATA_INDEX_FILE_TAG "metaidx"
#define ALTITUDES_FILE_TAG "altitudes"
#define ROAD_ACCESS_FILE_TAG "roadaccess"
#define RESTRICTIONS_FILE_TAG "restrictions"
#define ROUTING_FILE_TAG "routing"
#define CROSS_MWM_FILE_TAG "cross_mwm"
#define FEATURE_OFFSETS_FILE_TAG "offs"
#define RANKS_FILE_TAG "ranks"
#define REGION_INFO_FILE_TAG "rgninfo"
#define METALINES_FILE_TAG "metalines"
// Temporary addresses section that is used in search index generation.
#define SEARCH_TOKENS_FILE_TAG "addrtags"
#define TRAFFIC_KEYS_FILE_TAG "traffic"
#define TRANSIT_CROSS_MWM_FILE_TAG "transit_cross_mwm"
#define TRANSIT_FILE_TAG "transit"
#define UGC_FILE_TAG "ugc"

#define LOCALITY_DATA_FILE_TAG "locdata"
#define LOCALITY_INDEX_FILE_TAG "locidx"

#define READY_FILE_EXTENSION ".ready"
#define RESUME_FILE_EXTENSION ".resume"
#define DOWNLOADING_FILE_EXTENSION ".downloading"
#define BOOKMARKS_FILE_EXTENSION ".kml"
#define ROUTING_FILE_EXTENSION ".routing"
#define NOROUTING_FILE_EXTENSION ".norouting"
#define TRANSIT_FILE_EXTENSION ".transit.json"

#define GEOM_INDEX_TMP_EXT ".geomidx.tmp"
#define CELL2FEATURE_SORTED_EXT ".c2f.sorted"
#define CELL2FEATURE_TMP_EXT ".c2f.tmp"

#define LOCALITY_INDEX_TMP_EXT ".locidx.tmp"
#define CELL2LOCALITY_SORTED_EXT ".c2l.sorted"
#define CELL2LOCALITY_TMP_EXT ".c2l.tmp"

#define COUNTRIES_FILE "countries.txt"
#define COUNTRIES_META_FILE "countries_meta.txt"
#define COUNTRIES_OBSOLETE_FILE "countries_obsolete.txt"

#define WORLD_FILE_NAME "World"
#define WORLD_COASTS_FILE_NAME "WorldCoasts"
#define WORLD_COASTS_OBSOLETE_FILE_NAME "WorldCoasts_obsolete"

#define SETTINGS_FILE_NAME "settings.ini"
#define MARKETING_SETTINGS_FILE_NAME "marketing_settings.ini"

#define SEARCH_CATEGORIES_FILE_NAME "categories.txt"

#define PACKED_POLYGONS_INFO_TAG "info"
#define PACKED_POLYGONS_FILE "packed_polygons.bin"
#define PACKED_POLYGONS_OBSOLETE_FILE "packed_polygons_obsolete.bin"

#define EXTERNAL_RESOURCES_FILE "external_resources.txt"

#define GPS_TRACK_FILENAME "gps_track.dat"
#define RESTRICTIONS_FILENAME "restrictions.csv"
#define ROAD_ACCESS_FILENAME "road_access.csv"
#define METALINES_FILENAME "metalines.bin"

#define TRAFFIC_FILE_EXTENSION ".traffic"

#define REPLACED_TAGS_FILE "replaced_tags.txt"
#define MIXED_TAGS_FILE "mixed_tags.txt"
#define MIXED_NODES_FILE "mixed_nodes.txt"

#define LOCALIZATION_DESCRIPTION_SUFFIX " Description"

auto constexpr kInvalidRatingValue = -1.0f;
