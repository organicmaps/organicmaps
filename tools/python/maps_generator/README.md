# maps_generator

`maps_generator` is the Python CLI for generating `.mwm` maps for the Organic Maps application. This tool functions as the driver for the `generator_tool` C++ executable.

**Use the `generator_tool` and application from the same release. The application does not support
maps built by a generator_tool newer than the app.**

## What are maps?

Maps are `.mwm` binary files with special meta-information for rendering, searching, routing, and other use cases.
Files from [data/borders](https://github.com/organicmaps/organicmaps/tree/master/data/borders) define map boundaries for each individual file. The world is segmented into separate files by these boundaries, with the intent of having manageably small files to download. These files are referred to as *maps* or *countries*. A country is referring to one of these files, not necessarily a geographic country. Also note that there are two special countries called *World* and *WorldCoasts*. These are small simplified maps of the world and coastlines (sea and ocean watercover) used when other maps have not yet been downloaded.

## Setup

You must have Python version >= 3.7 and complete the following steps:

1. Switch to the branch of your app's version (see the note of #maps_generator section). E.g.:

```sh
git checkout 2023.06.04-13-android
```

The app version can be found in the "About" section of Organic Maps app.

2. Build the `generator_tool` binary (run from the root of the repo):

```sh
./tools/unix/build_omim.sh -r generator_tool
./tools/unix/build_omim.sh -r world_roads_builder_tool
./tools/unix/build_omim.sh -r mwm_diff_tool
```

3. Go to the `maps_generator` directory:

```sh
cd tools/python/maps_generator
```

4. Install python dependencies:

```sh
pip3 install -r requirements_dev.txt
```

5. Create a [configuration file with defaults](https://github.com/organicmaps/organicmaps/blob/master/tools/python/maps_generator/var/etc/map_generator.ini.default):

```sh
cp var/etc/map_generator.ini.default var/etc/map_generator.ini
```

6. Read through and edit the configuration file.

Ensure that `OMIM_PATH` is set correctly.
The default `PLANET_URL` setting makes the generator to download an OpenStreetMap dump file for the North Macedonia from [Geofabrik](http://download.geofabrik.de/index.html). Change `PLANET_URL` and `PLANET_MD5_URL` to get a region you want.

## Basic Usage

Make sure you are in the `tools/python` repo directory for starting the generator.

```sh
cd tools/python
```

Build a `.mwm` map file for North Macedonia without using coastlines (it's a land-locked country anyway):
```sh
python -m maps_generator --countries="Macedonia" --skip="Coastline"
```

It's possible to skip coastlines for countries that have a sea coast too, but the sea water will not be rendered in that case.

Make sure that requested countries are contained in your planet dump file indeed. Filenames in `data/borders/` (without the `.poly` extension) represent all valid country names.

To see other possible command-line options:
```sh
python -m maps_generator -h
```

If you are not from the Organic Maps team, then you do not need the `--production` option.

## Troubleshooting

The general log file (by default its `maps_build/generation.log`) contains output of the `maps_generator` python script only. More detailed logs that include output of the `generator_tool` binary are located in the `logs/` subdir of a particular build directory, e.g. `maps_build/2023_06_04__20_05_07/logs/`.

## More Examples

### Japan with coastlines

1. Open https://download.geofabrik.de/asia/japan.html and copy url of osm.pbf and md5sum files.
2. Put the urls into the `PLANET_URL` and `PLANET_MD5_URL` settings of the `map_generator.ini` file.
3. Set `PLANET_COASTS_URL` to a location with `latest_coasts.geom` and `latest_coasts.rawgeom` files. You don't need to download these files if the whole planet is built. They are generated in the process of building the whole planet (the coastline should be valid and continuous for it to succeed).
4. Run

```sh
python -m maps_generator --countries="World, WorldCoasts, Japan_*"
```

### Rebuild stages

For example, you changed routing code in the project and want to regenerate maps.
You must have previous generation. You may regenerate starting from the routing stage and only for two mwms:

```sh
python -m maps_generator -c --from_stage="Routing" --countries="Japan_Kinki Region_Osaka_Osaka, Japan_Chugoku Region_Tottori"
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
4. Extract this area from .osm.pbf file with the help of [osmium tool](https://osmcode.org/osmium-tool/):

```
osmium extract -p borders.geojson germany-latest.osm.pbf -o germany_part.osm.pbf
```

5. Run the `maps_generator` tool:

```sh
python -m maps_generator --skip="Coastline" --without_countries="World*"
```

In this example we skipped generation of the World\* files because they are ones of the most time- and resources-consuming mwms.

### Subways layer

You can manually generate a subway layer file to use in the `SUBWAY_URL` ini setting. See [instructions](https://github.com/organicmaps/organicmaps/tree/master/docs/SUBWAY_GENERATION.md).
