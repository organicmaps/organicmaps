# maps_generator

`maps_generator` is the Python CLI for generating .mwm maps for the Organic Maps application. This tool functions as the driver for the `generator_tool` C++ executable.

**Use the `generator_tool` and application from the same release. The application does not support
maps built by a generator_tool newer than the app.**

## What are maps?

Maps are .mwm binary files with special meta-information for rendering, searching, routing, and other use cases.
Files from [data/borders](https://github.com/organicmaps/organicmaps/tree/master/data/borders) define map boundaries for each individual file. The world is segmented into separate files by these boundaries, with the intent of having managebly small files to download. These files are referred to as *maps* or *countries*. A country is referring to one of these files, not necesarily a geographic country. Also note that there are two special countries called *World* and *WorldCoasts*. These are small simplified maps of the world and coastlines used when other maps have not yet been downloaded.

## Setup

You must have Python version >= 3.6 and complete the following steps:

1. Switch to the branch of your app's version (see the note of #maps_generator section).
   For example, if you use OMaps 9.2.3 you should do:

```sh
git checkout release-92
```

The app version can be found in the "About" section in the settings menu of OMaps.

2. Build and install the generator_tool.

```sh
./tools/unix/build_omim.sh -r generator_tool
./tools/unix/build_omim.sh -r world_roads_builder_tool
```

3. Change directory:

```sh
$ cd omim/tools/python/maps_generator
```

4. Install dependencies:

```sh
maps_generator$ pip3 install -r requirements_dev.txt
```

5. 1 Make the ini configuration file:

```sh
maps_generator$ cp var/etc/map_generator.ini.default var/etc/map_generator.ini
```

6. Edit the ini file:

```sh
maps_generator$ vim var/etc/map_generator.ini
```

Here is a sample ini that will download an OSM PBF file for the Yukon Territories, Canada from [GEOFABRIK](https://www.geofabrik.de/). You can replace the *osm.pbf* and *osm.pbf.md5* with other areas instead. Note that an entire planet file currently takes 40+ hours on a server with 256GB of RAM. Unless you have a machine this large, it is recommended to use a smaller extract.

```ini
[Main]
# If the flag DEBUG is set a special small planet file will be downloaded.
DEBUG: 0
# The path where the planet will be downloaded and the maps are generated.
MAIN_OUT_PATH: ~/maps_build
# The path where caches for nodes, ways, relations are stored.
# CACHE_PATH:


[Developer]
# The path where the generator_tool will be searched.
# Usually this is in the same parent directory of the organicmaps repo.
BUILD_PATH: ~/omim-build-release
# The path to the organicmaps repo
OMIM_PATH: ~/code/organicmaps


[Generator tool]
# The path to the data folder in the repository.
USER_RESOURCE_PATH: ${Developer:OMIM_PATH}/data

[Osm tools]
# The path to the osmctools sources.
OSM_TOOLS_SRC_PATH: ${Developer:OMIM_PATH}/tools/osmctools
# The path where osmctools will be searched or will be built.
OSM_TOOLS_PATH: ~/osmctools


[Stages]
# Run osmupdate tool for planet.
# You can set this to 1 if you would like osmupdate to apply updates to an out-of-date osm.pbf
NEED_PLANET_UPDATE: 0
# Auto detection.
THREADS_COUNT_FEATURES_STAGE: 0
# If you want to calculate diffs, you need to specify, where old maps are.
DATA_ARCHIVE_DIR: ${Generator tool:USER_RESOURCE_PATH}
# How many versions in the archive to use for diff calculation.
DIFF_VERSION_DEPTH: 2


[Logging]
# The path where maps_generator log will be saved.
# Defaults to $MAIN_OUT_PATH/generation.log
# LOG_FILE_PATH: generation.log


[External]
# Note: If you want to set a directory name you have to add "/" to the end of url.
# In each field where you need to specify a URL, you can specify the path to the file system using file:///path/to/file
# It is recommended to start with an https:// path to an osm.pbf and osm.pbf.mdf file.

# The url to the planet file.
PLANET_URL: https://download.geofabrik.de/north-america/canada/yukon-latest.osm.pbf
# The url to the file with md5 sum of the planet.
PLANET_MD5_URL: https://download.geofabrik.de/north-america/canada/yukon-latest.osm.pbf.md5
# The base url to WorldCoasts.geom and WorldCoasts.rawgeom (without file name).
# Files latest_coasts.geom and latest_coasts.rawgeom must be at this URL.
# For example, if PLANET_COASTS_URL = https://somesite.com/download/
# The https://somesite.com/download/latest_coasts.geom url will be used to download latest_coasts.geom and
# the https://somesite.com/download/latest_coasts.rawgeom url will be used to download latest_coasts.rawgeom.
# PLANET_COASTS_URL:
# Set to 'true' to build special routing section in World.mwm for alerting about absent regions without which the
# route can't be built. This should be true for any non-planet build.
NEED_BUILD_WORLD_ROADS: true
# The url to the subway file.
SUBWAY_URL: https://cdn.organicmaps.app/subway.json

# The url of the location with the transit files extracted from GTFS.
# TRANSIT_URL:

# Urls for production maps generation.
# UGC_URL:
# HOTELS_URL:
# PROMO_CATALOG_CITIES:
# POPULARITY_URL:
# FOOD_URL:
# FOOD_TRANSLATIONS_URL:
# SRTM_PATH:
# ISOLINES_PATH:
# UK_POSTCODES_URL:
# US_POSTCODES_URL:

[Common]
# Auto detection.
THREADS_COUNT: 0

[Stats]
# Path to rules for calculating statistics by type
STATS_TYPES_CONFIG: ${Developer:OMIM_PATH}/tools/python/maps_generator/var/etc/stats_types_config.txt
```

### Notes

In each field where you need to specify a URL, you can specify the path to the file system using `file:///path/to/file`.

You can manually generate a subway layer file with the SUBWAY_URL parameter. See [instructions](https://github.com/organicmaps/organicmaps/tree/master/docs/SUBWAY_GENERATION.md).

## Basic Usage

Make sure you are in the `tools/python` directory when running the CLI.  Unless you have URLs for coastline files, skip the coastline.

```sh
cd tools/python
python -m maps_generator --countries="Canada_Yukon_North, Canada_Yukon_Whitehorse" --skip="Coastline"
```

## Help

```sh
python$ python -m maps_generator -h
```

```
usage: __main__.py [-h] [--config CONFIG] [-c [CONTINUE]]
                   [--countries COUNTRIES] [--skip SKIP]
                   [--from_stage FROM_STAGE] [--production]

Tool for generation maps for maps.me application.

optional arguments:
  -h, --help            show this help message and exit
  --config CONFIG       Path to config
  -c [CONTINUE], --continue [CONTINUE]
                        Continue the last build or specified in CONTINUE from
                        the last stopped stage.
  -s SUFFIX, --suffix SUFFIX
                        Suffix of the name of a build directory.
  --countries COUNTRIES
                        List of regions, separated by a comma or a semicolon,
                        or path to file with regions, separated by a line
                        break, for which maps will be built. The names of the
                        regions can be seen in omim/data/borders. It is
                        necessary to set names without any extension.
  --without_countries WITHOUT_COUNTRIES
                        List of regions to exclude them from generation.
                        Syntax is the same as for --countries.
  --skip SKIP           List of stages, separated by a comma or a semicolon,
                        for which building will be skipped. Available skip
                        stages: DownloadAndConvertPlanet, UpdatePlanet,
                        Coastline, Preprocess, Features, Mwm, Index,
                        CitiesIdsWorld, Ugc, Popularity, Srtm, IsolinesInfo,
                        Descriptions, Routing, RoutingTransit, CountriesTxt,
                        ExternalResources, LocalAds, Statistics, Cleanup.
  --from_stage FROM_STAGE
                        Stage from which maps will be rebuild. Available
                        stages: DownloadAndConvertPlanet, UpdatePlanet,
                        Coastline, Preprocess, Features, Mwm, Index,
                        CitiesIdsWorld, Ugc, Popularity, Srtm, IsolinesInfo,
                        Descriptions, Routing, RoutingTransit, CountriesTxt,
                        ExternalResources, LocalAds, Statistics, Cleanup.
  --coasts              Build only WorldCoasts.raw and WorldCoasts.rawgeom
                        files
  --force_download_files
                        If build is continued, files will always be downloaded
                        again.
  --production          Build production maps. In another case, 'osm only
                        maps' are built - maps without additional data and
                        advertising.
  --order ORDER         Mwm generation order.
```

If you are not from the Organic Maps team, then you do not need the option --production when generating maps.

It is recommended to have 1TB of hard disk space with 256+GB of RAM to generate the entire planet. Expect the job to take about 40 hours.

Because different maps take varying amounts of time to generate, you can provide an ordered list of maps.
This way, you can choose which maps you would like to see completed first in a long build.
The default is `var/etc/mwm_generation_order.txt`, and you can override this behavior with the option 
`--order=/path/to/mwm_generation_order.txt`.
You can calculate this list yourself from the statistics, which is calculated with each generation.

## More Examples

### Japan with coastlines

1. Open https://download.geofabrik.de/asia/japan.html and copy url of osm.pbf and md5sum files.
2. Edit ini file:

```sh
maps_generator$ vim var/etc/map_generator.ini
```

```ini
[Main]
...
DEBUG: 0
...
[External]
PLANET_URL: https://download.geofabrik.de/asia/japan-latest.osm.pbf
PLANET_MD5_URL: https://download.geofabrik.de/asia/japan-latest.osm.pbf.md5
...
```

To build an entire country with coastlines, you need to download the *latest_coasts.geom* and *latest_coasts.rawgeom* files and specify their path in the config. You don't need to download these files if the whole planet is built. They are generated in the process of building the whole planet.

3. Run

```sh
python$ python -m maps_generator --countries="World, WorldCoasts, Japan_*"
```

### Rebuild stages

For example, you changed routing code in the project and want to regenerate maps.
You must have previous generation. You may regenerate from stage routing only for two mwms:

```sh
python$ python -m maps_generator -c --from_stage="Routing" --countries="Japan_Kinki Region_Osaka_Osaka, Japan_Chugoku Region_Tottori"
```

To generate maps with the coastline, you need more time and you need the planet to contain a continuous coastline.

### Extract without coastlines

If you want to generate maps for Moscow you must complete the following steps:

1. Open https://download.geofabrik.de/russia/central-fed-district.html and copy url of osm.pbf and md5sum files.
2. Edit ini file:

```sh
maps_generator$ vim var/etc/map_generator.ini
```

```ini
[Main]
...
DEBUG: 0
...
[External]
PLANET_URL: https://download.geofabrik.de/russia/central-fed-district-latest.osm.pbf
PLANET_MD5_URL: https://download.geofabrik.de/russia/central-fed-district-latest.osm.pbf.md5
...
```

3. Run

```sh
python$ python -m maps_generator --countries="Japan_Chugoku Region_Tottori" --skip="Coastline"
```

### Custom maps from GeoJSON

If you have an OSM PBF file and want to cut custom map regions, you can use a polygon feature in a GeoJSON file. This is a useful alternative if you want a custom area, or you do not want to figure out which countrie(s) apply to the area you need.

1. If you don't already have the .osm.pbf file, download applicable area of the world in .osm.pbf format, for example from [Geofabrik](http://download.geofabrik.de/index.html).
2. Generate area in geojson format of the territory in which you are interested. You can do it via [geojson.io](http://geojson.io/). Select the area on the map and copy corresponding part of the resulting geojson. You need to copy the contents of the `features: [ { ... } ]`, without features array, but with inner braces: `{...}`. For example, here is the full geojson of the rectangle area around Melbourne:

```json
{
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "properties": {},
      "geometry": {
        "type": "Polygon",
        "coordinates": [
          [
            [143.75610351562497, -39.21523130910491],
            [147.98583984375, -39.21523130910491],
            [147.98583984375, -36.03133177633187],
            [143.75610351562497, -36.03133177633187],
            [143.75610351562497, -39.21523130910491]
          ]
        ]
      }
    }
  ]
}
```

You need to copy this part of the geojson:

```json
{
  "type": "Feature",
  "properties": {},
  "geometry": {
    "type": "Polygon",
    "coordinates": [
      [
        [143.75610351562497, -39.21523130910491],
        [147.98583984375, -39.21523130910491],
        [147.98583984375, -36.03133177633187],
        [143.75610351562497, -36.03133177633187],
        [143.75610351562497, -39.21523130910491]
      ]
    ]
  }
}
```

3. Save selected geojson in some file with .geojson extension. For example, `borders.geojson`.
4. Extract this area from .osm.pbf file with the help of [osmium tool:](https://osmcode.org/osmium-tool/)

```
osmium extract -p borders.geojson germany-latest.osm.pbf -o germany_part.osm.pbf
```

5. Run the `maps_generator` tool:

```sh
python$ python -m maps_generator --skip="Coastline" --without_countries="World*"
```

In this example we skipped generation of the World\* files because they are ones of the most time- and resources-consuming mwms.
