#!/bin/bash
set -e -u -o pipefail

# Generate subways.transit.json file consumed by the maps generator.
# Inputs:
# - OSM planet in pbf format
# - csv table of subway networks
#   (auto-downloaded from https://docs.google.com/spreadsheets/d/1SEW1-NiNOnA2qDwievcxYV1FOaQl1mb1fdeyqAxHu3k)
# Output:
# - subways.transit.json

source "$(dirname "$0")/helper_settings.sh"
source "$REPO_PATH/tools/unix/helper_python.sh"

# Parameters for the process_subways.sh script:
export PLANET="$PLANET_PBF"
export SKIP_PLANET_UPDATE="1"
# http(s) or "file://" URL to a CSV file with a list of subway networks.
# Auto-downloaded from https://docs.google.com/spreadsheets/d/1SEW1-NiNOnA2qDwievcxYV1FOaQl1mb1fdeyqAxHu3k
# If unavailable then replace with a local file.
# TODO: keep the downloaded csv file from the latest run.
#export CITIES_INFO_URL=""
export TMPDIR="$BUILD_PATH/subways"
# The output file, which needs post-processing by transit_graph_generator.py
export MAPSME="$SUBWAYS_PATH/subways.json"

# Produce additional files needed for https://cdn.organicmaps.app/subway/
export HTML_DIR="$SUBWAYS_VALIDATOR_PATH"
export DUMP="$SUBWAYS_VALIDATOR_PATH"
export GEOJSON="$SUBWAYS_VALIDATOR_PATH"
export DUMP_CITY_LIST="$SUBWAYS_VALIDATOR_PATH/cities.txt"

"$SUBWAYS_REPO_PATH/scripts/process_subways.sh" 2>&1 | tee "$SUBWAYS_LOG"

# Make render.html available for map visualization on the web
cp -r "$SUBWAYS_REPO_PATH"/render/* "$SUBWAYS_VALIDATOR_PATH/"

TRANSIT_TOOL_PATH="$REPO_PATH/tools/python/transit"
SUBWAYS_GRAPH_FILE="$SUBWAYS_PATH/subways.transit.json"

activate_venv_at_path "$TRANSIT_TOOL_PATH"
"$PYTHON" "$TRANSIT_TOOL_PATH/transit_graph_generator.py" "$MAPSME" "$SUBWAYS_GRAPH_FILE" 2>&1 | tee -a "$SUBWAYS_LOG"
deactivate

echo "Generated subways transit graph file:"
echo "$SUBWAYS_GRAPH_FILE"
echo "Finished"
