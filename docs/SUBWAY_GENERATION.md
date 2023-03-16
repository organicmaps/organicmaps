# Subway layer generation

For the subway layer to be available in Organic Maps a `SUBWAY_URL`
parameter ought to be provided to the
[map generator](https://github.com/organicmaps/organicmaps/tree/master/tools/python/maps_generator).
Normally you can specify the link https://cdn.organicmaps.app/subway.json
to a regularily updatable file.
This instruction describes how to manually build subway layer file.

Japan is used as target country but any other region from one city to the
whole planet is also applicable.

1.  Make sure you have an \*.o5m file with the region. As an option, you can
    download the region in \*.osm.pbf format and convert it to \*.o5m:

    ```bash
    wget https://download.geofabrik.de/asia/japan-latest.osm.pbf
    osmconvert japan-latest.osm.pbf -o=japan.o5m --drop-version
    ```

    If you already have some not too outdated \*.o5m version, it is enough
    because the subway generation script will update it with `osmupdate`.

1.  With [Organic Maps subways](https://github.com/organicmaps/subways) repository deployed,
    run `scripts/process_subways.sh` bash script or prepare your own script
    which launches `process_subways.py` and `validation_to_html.py` scripts
    with suitable options.

1.  Analyze HTML output obtained at the previous step. Broken transport
    networks won't be included into the map.

1.  Run `tools/python/transit/transit_graph_generator.py` from `omim`.

The last three steps may be expressed as the following shell script:

```bash
#!/bin/bash
set -e

REPO="/path/to/cloned/repo"
export PLANET="/path/to/japan.o5m"
export OSMCTOOLS="/path/to/osmctools/dir"
export HTML_DIR="/generated/html/target/dir"
export PYTHON=python36
export TMPDIR="/tmp"
# Set non-empty string for the script to update validator code from git repository
export GIT_PULL=
export JSON=$TMPDIR/omaps_osm_subways.json
# Set DUMP variable if YAML files needed.
#export DUMP="$HTML_DIR"
# Put city/country name to CITY variable if not all networks needed.
# The default source of available cities/countries you may find at subways repository README.
#export CITY="Germany"

bash -x "$REPO/subways/scripts/process_subways.sh"

TRANSIT_MAPS=${JSON%".json"}.transit.json
cd "$REPO/tools/python/transit"
"$PYTHON" transit_graph_generator.py "$JSON" "$TRANSIT_MAPS"
```

It is the `$TRANSIT_MAPS` file that can be fed to the map generator via `SUBWAY_URL` parameter.
