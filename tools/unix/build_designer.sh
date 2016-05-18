#!/bin/bash
set -e -u

# Prepare environment variables which specify app version and codebase sha
APP_VERSION=UNKNOWN
[ $# -gt 0 ] && APP_VERSION=$1
DESIGNER_CODEBASE_SHA=$(git log -1 --format="%H")
OMIM_PATH="$(cd "${OMIM_PATH:-$(dirname "$0")/../..}"; pwd)"
DATA_PATH="$OMIM_PATH/data"
BUILD_PATH="$OMIM_PATH/out"
RELEASE_PATH="$BUILD_PATH/release"

source "$OMIM_PATH/tools/autobuild/detect_qmake.sh"

# Print designer_version.h file
cat > "$OMIM_PATH/designer_version.h" <<DVER
#pragma once
#define DESIGNER_APP_VERSION "$APP_VERSION"
#define DESIGNER_CODEBASE_SHA "$DESIGNER_CODEBASE_SHA"
#define DESIGNER_DATA_VERSION ""
DVER

rm -rf "$RELEASE_PATH"
(
  cd "$OMIM_PATH"
  ${QMAKE-qmake} omim.pro -r -spec macx-clang CONFIG+=release CONFIG+=x86_64 CONFIG+=map_designer CONFIG+=no-tests
  make -j8
)

# Prepare app package by copying Qt, Kothic, Skin Generator, Style tests
for i in skin_generator style_tests generator_tool MAPS.ME.Designer; do
  "$(dirname "$QMAKE")/macdeployqt" "$RELEASE_PATH/$i.app"
done

MAC_RESOURCES="$RELEASE_PATH/MAPS.ME.Designer.app/Contents/Resources"
cp -r "$RELEASE_PATH/style_tests.app" "$MAC_RESOURCES/style_tests.app"
cp -r "$RELEASE_PATH/skin_generator.app" "$MAC_RESOURCES/skin_generator.app"
cp -r "$RELEASE_PATH/generator_tool.app" "$MAC_RESOURCES/generator_tool.app"
cp -r "$OMIM_PATH/tools/kothic" "$MAC_RESOURCES/kothic"
cp "$OMIM_PATH/tools/python/stylesheet/drules_info.py" "$MAC_RESOURCES/kothic/src/drules_info.py"
cp "$OMIM_PATH/protobuf/protobuf-2.6.1-py2.7.egg" "$MAC_RESOURCES/kothic"
cp "$OMIM_PATH/tools/python/recalculate_geom_index.py" "$MAC_RESOURCES/recalculate_geom_index.py"

# Copy all drules and  resources (required for test environment)
rm -rf $MAC_RESOURCES/drules_proto*
rm -rf $MAC_RESOURCES/resources-*
for i in ldpi mdpi hdpi xhdpi xxhdpi 6plus; do
  cp -r $OMIM_PATH/data/resources-${i}_legacy/ $MAC_RESOURCES/resources-$i/
done
cp $OMIM_PATH/data/drules_proto_legacy.bin $MAC_RESOURCES/drules_proto.bin
for i in resources-default countries-strings cuisine-strings WorldCoasts_obsolete.mwm countries.txt cuisines.txt countries_obsolete.txt packed_polygons.bin packed_polygons_obsolete.bin; do
  cp -r $OMIM_PATH/data/$i $MAC_RESOURCES/
done

# Build DMG image
rm -rf "$BUILD_PATH/deploy"
mkdir "$BUILD_PATH/deploy"
cp -r "$RELEASE_PATH/MAPS.ME.Designer.app" "$BUILD_PATH/deploy/MAPS.ME.Designer.app"
cp -r "$DATA_PATH/styles" "$BUILD_PATH/deploy/styles"

DMG_NAME=MAPS.ME.Designer.$APP_VERSION
hdiutil create -size 640m -volname $DMG_NAME -srcfolder "$BUILD_PATH/deploy" -ov -format UDZO "$BUILD_PATH/$DMG_NAME.dmg"
