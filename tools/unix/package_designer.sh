#!/usr/bin/env bash
# Assembles a portable Organic Maps Designer package from an already built desktop tree:
# the app itself, the binaries and Python scripts it shells out to, and the MapCSS sources
# to edit.  CI publishes it as a downloadable artifact, see docs/STYLES.md.
#
# Usage: tools/unix/package_designer.sh <build-dir> [<output-dir>]

set -euo pipefail

if [ $# -lt 1 ]; then
  echo "Usage: ${0##*/} <build-dir> [<output-dir>]" >&2
  exit 1
fi

OMIM_PATH="${OMIM_PATH:-$(cd "$(dirname "$0")/../.."; pwd)}"
BUILD_DIR="$(cd "$1"; pwd)"
OUT_DIR="${2:-$BUILD_DIR/OrganicMaps-Designer}"

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

rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

echo "Copying $APP and the binaries the Designer runs"
cp -R "$BUILD_DIR/$APP" "$OUT_DIR/"
# Recalculate geometry index runs generator_tool, Run tests runs style_tests.
# GetExternalPath() finds both next to the app, see qt/build_style/build_common.cpp.
for binary in generator_tool style_tests; do
  if [ ! -x "$BUILD_DIR/$binary" ]; then
    echo "No $BUILD_DIR/$binary, build the '$binary' target first." >&2
    exit 2
  fi
  cp "$BUILD_DIR/$binary" "$OUT_DIR/"
done

echo "Copying data"
# <package>/data is the writable dir the app finds next to the binary (Linux) or next to the
# bundle (macOS), so it is both where the MapCSS sources to edit live and where Build Style
# writes the rebuilt drules and symbols.
if [ "$APP" = OrganicMaps.app ]; then
  # The bundle's Resources already carry every runtime resource (copy_resources() in
  # qt/CMakeLists.txt), so ship only what the Designer adds on top of them.
  DATA_PATHSPEC=(data/styles data/mapcss-mapping.csv data/mapcss-dynamic.txt)
else
  # Everything except the generator- and test-only data, which is most of data/ by size.
  DATA_PATHSPEC=(data ':(exclude)data/borders' ':(exclude)data/test_data' ':(exclude)data/minsk-pass.*'
                 ':(exclude)data/*.md')
fi
# ls-files lists the checked-in data only, skipping maps a dev checkout may have downloaded.
git -C "$OMIM_PATH" ls-files -z -- "${DATA_PATHSPEC[@]}" \
  | tar -c -C "$OMIM_PATH" --null -T - -f - \
  | tar -x -C "$OUT_DIR" -f -

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
  MACDEPLOYQT="${QT_PATH:+$QT_PATH/bin/macdeployqt}"
  if [ ! -x "$MACDEPLOYQT" ]; then
    MACDEPLOYQT="$(command -v macdeployqt || true)"
  fi
  if [ -z "$MACDEPLOYQT" ]; then
    echo "macdeployqt not found, set QT_PATH to the Qt 6 installation." >&2
    exit 2
  fi
  "$MACDEPLOYQT" "$OUT_DIR/$APP"
  # macdeployqt leaves the frameworks it rewrote with a stale signature and still exits with 0,
  # so re-sign and verify here: Gatekeeper refuses to open a downloaded app that fails to verify.
  codesign --force --sign - --deep "$OUT_DIR/$APP"
  codesign --verify --deep --strict "$OUT_DIR/$APP"
  # Nothing sets CMAKE_OSX_DEPLOYMENT_TARGET, so the app runs on the macOS it was built on
  # and newer only; Homebrew's Qt, bundled above, is built per macOS version anyway.
  REQUIREMENT="macOS $(otool -l "$OUT_DIR/$APP_BINARY" | awk '/minos/ {print $2; exit}') or newer.
  Qt is bundled, but the package is not notarized, so clear the download quarantine flag
  once after unpacking: \`xattr -dr com.apple.quarantine .\`"
else
  REQUIREMENT="Ubuntu 24.04 or newer with Qt 6 installed system-wide (Qt is not bundled):
  \`sudo apt install qt6-base-dev qt6-positioning-dev libqt6svg6-dev\`, as in docs/INSTALL.md."
fi

cat > "$OUT_DIR/designer.sh" <<EOF
#!/usr/bin/env bash
# Opens the Designer on a style.mapcss, data/styles/default/light/style.mapcss by default.
set -euo pipefail
cd "\$(dirname "\$0")"
STYLE="\${1:-data/styles/default/light/style.mapcss}"
case "\$STYLE" in /*) ;; *) STYLE="\$PWD/\$STYLE" ;; esac
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

See https://github.com/organicmaps/organicmaps/blob/master/docs/STYLES.md for the full guide.
EOF

echo "Designer package is ready in $OUT_DIR"
