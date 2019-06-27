# Subway layer generation

For the subway layer to be available on Maps.Me maps a `SUBWAY_URL` 
parameter ought to be provided to the
[map generator](https://github.com/mapsme/omim/tree/master/tools/python/maps_generator).
Normally you can specify the link http://osm-subway.maps.me/mapsme/latest.json
to a regularily updatable file. 
This instruction describes how to manually build subway layer file.


Japan is used as target country but any other region from one city to the
whole planet is also applicable.

1. Make sure you have an *.o5m file with the region. As an option, you can
download the region in *.osm.pbf format and convert it to *.o5m:
 
    `wget https://download.geofabrik.de/asia/japan-latest.osm.pbf`\
    `osmconvert japan-latest.osm.pbf -o=japan.o5m --drop-version`\
    \
If you already have some not too outdated *.o5m version, it is enough
because the subway generation script will update it with `osmupdate`. 

1. With [Maps.Me subways](https://github.com/mapsme/subways) repository deployed,
run `scripts/process_subways.sh` bash script or prepare your own script
which launches `process_subways.py` and `validation_to_html.py` scripts
with suitable options. 
 
1. Analyze HTML output obtained at the previous step. Broken transport
networks won't be included into the map.

1. Run `tools/python/transit/transit_graph_generator.py` from `omim`.

The last three steps may be expressed as the following shell script:

```bash
#!/bin/bash
set -e

export PLANET="/path/to/japan.o5m"
export OSMCTOOLS="/path/to/osmctools/dir"
export HTML_DIR="/generated/html/target/dir"
export PYTHON=python36
export TMPDIR="/tmp"
# Set non-empty string for the script to update validator code from git repository
export GIT_PULL=
export MAPSME=$TMPDIR/mapsme_osm_subways.json
# Set DUMP variable if YAML files needed.
#export DUMP="$HTML_DIR"
# Put city/country name to CITY variable if not all networks needed.
# The default source of available cities/countries you may find at subways repository README.
#export CITY="Germany"

bash -x "/path/to/cloned/repo/subways/"scripts/process_subways.sh

TRANSIT_MAPSME=${MAPSME%".json"}.transit.json
cd "/path/to/cloned/repo/omim/"tools/python/transit
"$PYTHON" transit_graph_generator.py "$MAPSME" "$TRANSIT_MAPSME"
```

It is the `$TRANSIT_MAPSME` file
that can be feed to map generator via `SUBWAY_URL` parameter.

 
