#!/usr/bin/env bash
# Assembles a portable Organic Maps Designer package from an already built desktop tree:
# the app itself, the binaries and Python scripts it shells out to, and the MapCSS sources
# to edit.  CI publishes it as a downloadable artifact, see docs/STYLES.md.
#
# Usage: tools/unix/package_designer.sh <build-dir> [<output-parent-dir>]

set -euo pipefail

if [ $# -lt 1 ]; then
  echo "Usage: ${0##*/} <build-dir> [<output-parent-dir>]" >&2
  exit 1
fi

OMIM_PATH="${OMIM_PATH:-$(cd "$(dirname "$0")/../.." && pwd)}"
BUILD_DIR="$(cd "$1" && pwd)"
OUT_DIR="${2:-$BUILD_DIR}/OrganicMaps-Designer"

# The desktop build produces a bundle on macOS and an executable on Linux.
if [ "$(uname -s)" = Darwin ]; then
  APP=OrganicMaps.app
  APP_BINARY="$APP/Contents/MacOS/OrganicMaps"
else
  APP=OrganicMaps
  APP_BINARY="$APP"
fi

if [ ! -x "$BUILD_DIR/$APP_BINARY" ]; then
  echo "No $BUILD_DIR/$APP_BINARY, build the 'desktop' target first." >&2
  exit 2
fi

for binary in generator_tool style_tests; do
  if [ ! -x "$BUILD_DIR/$binary" ]; then
    echo "No $BUILD_DIR/$binary, build the '$binary' target first." >&2
    exit 2
  fi
done

LICENSE_FILES=(LICENSE NOTICE DATA_LICENSE.txt LEGAL CONTRIBUTORS)
for name in "${LICENSE_FILES[@]}"; do
  if [ ! -f "$OMIM_PATH/$name" ]; then
    echo "Missing license input: $OMIM_PATH/$name" >&2
    exit 2
  fi
done
if [ ! -d "$OMIM_PATH/LICENSES" ]; then
  echo "Missing license directory: $OMIM_PATH/LICENSES" >&2
  exit 2
fi

if [ "$APP" = OrganicMaps.app ]; then
  MACDEPLOYQT="${QT_PATH:+$QT_PATH/bin/macdeployqt}"
  if [ ! -x "$MACDEPLOYQT" ]; then
    MACDEPLOYQT="$(command -v macdeployqt || true)"
  fi
  if [ -z "$MACDEPLOYQT" ]; then
    echo "macdeployqt not found, set QT_PATH to the Qt 6 installation." >&2
    exit 2
  fi
fi

rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

echo "Copying $APP and the binaries the Designer runs"
cp -R "$BUILD_DIR/$APP" "$OUT_DIR/"
# Recalculate geometry index runs generator_tool, Run tests runs style_tests.
HELPER_DIR="$OUT_DIR"
if [ "$APP" = OrganicMaps.app ]; then
  HELPER_DIR="$OUT_DIR/$APP/Contents/MacOS"
fi
for binary in generator_tool style_tests; do
  cp "$BUILD_DIR/$binary" "$HELPER_DIR/"
done

echo "Copying data"
# <package>/data is the writable dir the app finds next to the binary (Linux) or next to the
# bundle (macOS), so it is both where the MapCSS sources to edit live and where Build Style
# writes the rebuilt drules and symbols.
if [ "$APP" = OrganicMaps.app ]; then
  # The bundle carries the shared runtime resources. Style sources and generator-only inputs
  # belong in the writable package data directory, where helper readers also look.
  DATA_PATHSPEC=(data/styles data/mapcss-mapping.csv data/mapcss-dynamic.txt data/generator/timezone/timezone_info.json)
else
  # Everything except the generator- and test-only data, which is most of data/ by size.
  DATA_PATHSPEC=(data ':(exclude)data/borders' ':(exclude)data/test_data' ':(exclude)data/minsk-pass.*'
                 ':(exclude)data/*.md')
fi
# ls-files lists the checked-in data only, skipping maps a dev checkout may have downloaded.
git -C "$OMIM_PATH" ls-files -z -- "${DATA_PATHSPEC[@]}" \
  | tar -c -C "$OMIM_PATH" --null -T - -f - \
  | tar -x -C "$OUT_DIR" -f -

echo "Copying licenses"
mkdir -p "$OUT_DIR/licenses"
for name in "${LICENSE_FILES[@]}"; do
  cp "$OMIM_PATH/$name" "$OUT_DIR/licenses/"
done
cp -R "$OMIM_PATH/LICENSES" "$OUT_DIR/licenses/"

echo "Copying Python tools"
# libkomwm.py and merge_variants.py compile the MapCSS, drules_info.py backs Get statistics
# and recalculate_geom_index.py backs Recalculate geometry index.
mkdir -p "$OUT_DIR/tools/kothic" "$OUT_DIR/tools/python"
cp -R "$OMIM_PATH/tools/kothic/src" "$OUT_DIR/tools/kothic/"
cp -R "$OMIM_PATH/tools/python/stylesheet" "$OUT_DIR/tools/python/"
cp "$OMIM_PATH/tools/python/recalculate_geom_index.py" "$OUT_DIR/tools/python/"
find "$OUT_DIR/tools" -name __pycache__ -type d -exec rm -rf {} +

if [ "$APP" = OrganicMaps.app ]; then
  echo "Bundling Qt into $APP"
  "$MACDEPLOYQT" "$OUT_DIR/$APP" \
    -executable="$HELPER_DIR/generator_tool" -executable="$HELPER_DIR/style_tests"
  # macdeployqt leaves the frameworks it rewrote with a stale signature and still exits with 0,
  # so re-sign and verify here: Gatekeeper refuses to open a downloaded app that fails to verify.
  codesign --force --sign - --deep "$OUT_DIR/$APP"
  codesign --verify --deep --strict "$OUT_DIR/$APP"
  # Nothing sets CMAKE_OSX_DEPLOYMENT_TARGET, so the app runs on the macOS it was built on
  # and newer only; Homebrew's Qt, bundled above, is built per macOS version anyway.
  REQUIREMENT="macOS $(otool -l "$OUT_DIR/$APP_BINARY" | awk '/minos/ {print $2; exit}') or newer.
  Qt is bundled for the app and its helpers. The package is not notarized, so clear the download quarantine flag
  once after unpacking: \`xattr -dr com.apple.quarantine .\`"
else
  REQUIREMENT="Linux with compatible system Qt 6 and C++ runtime libraries (Qt is not bundled).
  The CI Linux artifact is built on Ubuntu 26.04 with GCC 15; use the matching distribution:
  \`sudo apt install qt6-base-dev qt6-positioning-dev libqt6svg6-dev\`, as in docs/INSTALL.md."
fi

cat > "$OUT_DIR/designer.sh" <<EOF
#!/usr/bin/env bash
# Opens the Designer on a style.mapcss, data/styles/default/light/style.mapcss by default.
set -euo pipefail
cd "\$(dirname "\$0")"
STYLE="\${1:-data/styles/default/light/style.mapcss}"
case "\$STYLE" in /*) ;; *) STYLE="\$PWD/\$STYLE" ;; esac
if [ -d ./OrganicMaps.app/Contents/Resources ]; then
  # Explicit paths also apply to console helpers before their command-line flags are parsed.
  export MWM_RESOURCES_DIR="\$PWD/OrganicMaps.app/Contents/Resources"
  export MWM_WRITABLE_DIR="\$PWD/data"
fi
exec ./$APP_BINARY --designer="\$STYLE"
EOF
chmod +x "$OUT_DIR/designer.sh"

cat > "$OUT_DIR/README.md" <<EOF
# Organic Maps Designer

Edit the map styles and see the result immediately, without building Organic Maps.
Built from [$(git -C "$OMIM_PATH" rev-parse --short HEAD)](https://github.com/organicmaps/organicmaps/commit/$(git -C "$OMIM_PATH" rev-parse HEAD)).

## Requirements

- \`python3\` (no extra packages: the MapCSS compiler is pure Python).
- $REQUIREMENT

## Usage

\`\`\`bash
./designer.sh                                          # data/styles/default/light/style.mapcss
./designer.sh data/styles/outdoors/dark/style.mapcss   # or any of the other five styles
\`\`\`

Edit any file under \`data/styles/<type>/\` and press **Build style** to recompile the
drawing rules and symbol atlases and reload them in the running app.  \`data/styles/\` is a
copy of the repository's, so \`diff\` or \`git apply\` moves finished edits back into a checkout.

Keep the app inside this folder: it locates \`data/\` and \`tools/\` relative to itself.
License and attribution texts are included in \`licenses/\`.

See https://github.com/organicmaps/organicmaps/blob/master/docs/STYLES.md for the full guide.
EOF

echo "Designer package is ready in $OUT_DIR"
