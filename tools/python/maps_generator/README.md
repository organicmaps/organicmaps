# maps_generator
It's tool for generation maps for maps.me application.

Note: **Use generator_tool and application from the same release. Application does not support
maps built by generator_tool newer than app.**

##  What are maps for maps.me?
Maps for maps.me are data with special meta information for drawing, quick searching and routing and for other.
Files from [omim/data/borders](https://github.com/mapsme/omim/tree/master/data/borders) define map boundaries.
Further we will call it countries or simply maps.
But there are two special maps: World and WorldCoasts. They are used when other maps have not been downloaded.
Further we will call them world and coastlines.

## Setup
You must have Python version not lower than 3.6 and complete the following steps:

0. Switch to the branch of your app's version (see the note of #maps_generator section). 
For example, if you use MAPS.ME 9.2.3 you should do:
```sh
git checkout release-92
```
The app version can be found in the "About" section in the settings menu of MAPS.ME.
1. [Build and install generator_tool.](https://github.com/mapsme/omim/blob/master/docs/INSTALL.md#maps-generator)
2. Change directory:
```sh
$ cd omim/tools/python/maps_generator
```
3. Install dependencies:
```sh
maps_generator$ pip3 install -r requirements_dev.txt
```

4. Make ini file:
```sh
maps_generator$ cp var/etc/map_generator.ini.default var/etc/map_generator.ini
```

5. Edit ini file:
```sh
maps_generator$ vim var/etc/map_generator.ini
```

```ini
[Main]
# If the flag DEBUG is set a special small planet file will be downloaded.
DEBUG: 1
# The path where the planet will be downloaded and the maps are generated.
MAIN_OUT_PATH: ~/maps_build
# The path where caches for nodes, ways, relations are stored.
# CACHE_PATH:


[Developer]
# The path where the generator_tool will be searched.
BUILD_PATH: ~/omim-build-release
# The path to the project directory omim.
OMIM_PATH: ~/omim


[Generator tool]
# The path to the omim/data.
USER_RESOURCE_PATH: ${Developer:OMIM_PATH}/data
# Do not change it. This is determined automatically.
# NODE_STORAGE: map


[Osm tools]
# The path to the osmctools sources.
OSM_TOOLS_SRC_PATH: ${Developer:OMIM_PATH}/tools/osmctools
# The path where osmctools will be searched or will be built.
OSM_TOOLS_PATH: ~/osmctools


[Stages]
# Run osmupdate tool for planet.
NEED_PLANET_UPDATE: 0
# Auto detection.
THREADS_COUNT_FEATURES_STAGE: 0
# If you want to calculate diffs, you need to specify, where old maps are
DATA_ARCHIVE_DIR: ${Generator tool:USER_RESOURCE_PATH}
# You may specify, how many versions in the archive to use for diff calculation
DIFF_VERSION_DEPTH: 2


[Logging]
# The path where maps_generator log will be saved.
# LOG_FILE_PATH: generation.log


[External]
# The url to the planet file.
# PLANET_URL:
# The url to the file with md5 sum of the planet.
# PLANET_MD5_URL:
# The base url to WorldCoasts.geom and WorldCoasts.rawgeom (without file name).
# Files latest_coasts.geom and latest_coasts.rawgeom must be at this URL.
# For example, if PLANET_COASTS_URL = https://somesite.com/download/
# The https://somesite.com/download/latest_coasts.geom url will be used to download latest_coasts.geom and
# the https://somesite.com/download/latest_coasts.rawgeom url will be used to download latest_coasts.rawgeom.
# PLANET_COASTS_URL:
# The url to the subway file.
SUBWAY_URL: http://osm-subway.maps.me/mapsme/latest.json

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


##### Note 1: In each field where you need to specify a URL, you can specify the path to the file system using file:///path/to/file

##### Note 2: You can manually generate subway layer file for SUBWAY_URL parameter. See [instructions](https://github.com/mapsme/omim/tree/master/docs/SUBWAY_GENERATION.md).

## Usage
```sh
$ cd omim/tools/python
python$ python3.6 -m maps_generator -h
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

If you are not from the maps.me team, then you do not need the option --production when generating maps.

To generate maps for the whole planet you need 400 GB of hard disk space and a computer with more than 64 GB RAM.


If you want to generate a lot of maps, then it may be important for you to order the generation of maps.
Because different maps take different amounts of time to generate.
Using a list with maps order can reduce build time on a multi-core computer.
The order from: var/etc/mwm_generation_order.txt is used by default.
You can override this behavior with the option --order=/path/to/mwm_generation_order.txt
You can calculate this list yourself from the statistics, which is calculated with each generation.

### Examples
####  Non-standard planet with coastlines
If you want to generate maps for Japan you must complete the following steps:
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

3. Run
```sh
python$ python3.6 -m maps_generator --countries="World, WorldCoasts, Japan_*"
```

####  Rebuild stages:
For example, you changed routing code in omim project and want to regenerate maps.
You must have previous generation. You may regenerate from stage routing only for two mwms:

```sh
python$ python3.6 -m maps_generator -c --from_stage="Routing" --countries="Japan_Kinki Region_Osaka_Osaka, Japan_Chugoku Region_Tottori"
```
##### Note: To generate maps with the coastline, you need more time and you need the planet to contain a continuous coastline.

####  Non-standard planet without coastlines
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
python$ python3.6 -m maps_generator --countries="Russia_Moscow" --skip="Coastline"
```

#### Generate all possible mwms from .osm.pbf file
If you have some .osm.pbf file, want to cut some area from it and generate maps from this area, but don't want to think what mwms got into this .osm.pbf file, you may follow the steps:
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
            [
              143.75610351562497,
              -39.21523130910491
            ],
            [
              147.98583984375,
              -39.21523130910491
            ],
            [
              147.98583984375,
              -36.03133177633187
            ],
            [
              143.75610351562497,
              -36.03133177633187
            ],
            [
              143.75610351562497,
              -39.21523130910491
            ]
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
            [
              143.75610351562497,
              -39.21523130910491
            ],
            [
              147.98583984375,
              -39.21523130910491
            ],
            [
              147.98583984375,
              -36.03133177633187
            ],
            [
              143.75610351562497,
              -36.03133177633187
            ],
            [
              143.75610351562497,
              -39.21523130910491
            ]
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
python$ python3.6 -m maps_generator --skip="Coastline" --without_countries="World*"
```
In this example we skipped generation of the World* files because they are ones of the most time- and resources-consuming mwms.
