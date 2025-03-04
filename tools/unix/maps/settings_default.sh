# NOTE: edit the settings.sh file to customize/override the defaults.

# Absolutize & normalize paths.
REPO_PATH="${REPO_PATH:-$(cd "$(dirname "$0")/../../.."; pwd -P)}"

BASE_PATH="${BASE_PATH:-$REPO_PATH/../maps}"
# Temporary files
BUILD_PATH="${BUILD_PATH:-$BASE_PATH/build}"
# Other code repositories, e.g. subways, wikiparser..
CODE_PATH="${CODE_PATH:-$REPO_PATH/..}"
# Source map data and processed outputs e.g. wiki articles
DATA_PATH="${DATA_PATH:-$BASE_PATH/data}"

# OSM planet source files

PLANET_PATH="${PLANET_PATH:-$DATA_PATH/planet}"
PLANET_PBF="${PLANET_PBF:-$PLANET_PATH/planet-latest.osm.pbf}"
PLANET_O5M="${PLANET_O5M:-$PLANET_PATH/planet-latest.o5m}"

# Subways

SUBWAYS_REPO_PATH="${SUBWAYS_REPO_PATH:-$CODE_PATH/subways}"
SUBWAYS_PATH="${SUBWAYS_PATH:-$DATA_PATH/subways}"
SUBWAYS_LOG="${SUBWAYS_LOG:-$SUBWAYS_PATH/subways.log}"
SUBWAYS_VALIDATOR_PATH="${SUBWAYS_VALIDATOR_PATH:-$SUBWAYS_PATH/validator}"
