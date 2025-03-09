SETTINGS_FILE="${SETTINGS_FILE:-$(cd "$(dirname "$0")"; pwd -P)/settings.sh}"
if [ -f "$SETTINGS_FILE" ]; then
  echo "Using settings from $SETTINGS_FILE"
  source "$SETTINGS_FILE"
else
  echo "Creating a template settings file $SETTINGS_FILE"
  cat << EOF > "$SETTINGS_FILE"
# Customize the default settings here.
# (check the defaults in settings_default.sh)

# The default maps workspace base is ../maps relative to the repo.
# All source and output and intermediate files will be organized there in corresponding subdirs.
# E.g. set it to the user's home directory:
# BASE_PATH="$HOME"
EOF
fi

source "$(dirname "$0")/settings_default.sh"

mkdir -p "$BASE_PATH"
mkdir -p "$BUILD_PATH"
mkdir -p "$DATA_PATH"

mkdir -p "$PLANET_PATH"
mkdir -p "$SUBWAYS_PATH"
