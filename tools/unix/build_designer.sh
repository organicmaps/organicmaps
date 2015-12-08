#!/bin/bash

set -e -u

# Prepare environment variables which specify app version and codebase sha
APP_VERSION=UNKNOWN
[ $# -gt 0 ] && APP_VERSION=$1
DESIGNER_CODEBASE_SHA=$(git log -1 --format="%H")

echo "App version: $APP_VERSION"
echo "Commit SHA: $DESIGNER_CODEBASE_SHA"

OMIM_PATH="$(dirname "$0")/../.."
DATA_PATH="$OMIM_PATH/data"
BUILD_PATH="$OMIM_PATH/out"
RELEASE_PATH="$BUILD_PATH/release"

# Print designer_version.h file
cat > "$OMIM_PATH/designer_version.h" <<DVER
#pragma once
#define DESIGNER_APP_VERSION "$APP_VERSION"
#define DESIGNER_CODEBASE_SHA "$DESIGNER_CODEBASE_SHA"
#define DESIGNER_DATA_VERSION ""
DVER

# Substitute tools/autobuild/build.sh
rm "$OMIM_PATH/tools/autobuild/build.sh"
cp "$OMIM_PATH/tools/autobuild/build_designer.sh" "$OMIM_PATH/tools/autobuild/build.sh"

rm -rf "$RELEASE_PATH"
(
  cd "$OMIM_PATH"
  ${QMAKE-qmake} omim.pro -r -spec macx-clang CONFIG+=release CONFIG+=x86_64 CONFIG+=map_designer
  make -j8
)

# Prepare app package by copying Qt, Kothic, Skin Generator, Style tests
macdeployqt "$RELEASE_PATH/skin_generator.app"
macdeployqt "$RELEASE_PATH/style_tests.app"
macdeployqt "$RELEASE_PATH/generator_tool.app"
macdeployqt "$RELEASE_PATH/MAPS.ME.Designer.app"
MAC_RESOURCES="$RELEASE_PATH/MAPS.ME.Designer.app/Contents/Resources"
cp -r "$RELEASE_PATH/style_tests.app" "$MAC_RESOURCES/style_tests.app"
cp -r "$RELEASE_PATH/skin_generator.app" "$MAC_RESOURCES/skin_generator.app"
cp -r "$RELEASE_PATH/generator_tool.app" "$MAC_RESOURCES/generator_tool.app"
cp -r "$OMIM_PATH/tools/kothic" "$MAC_RESOURCES/kothic"
cp "$OMIM_PATH/tools/python/stylesheet/drules_info.py" "$MAC_RESOURCES/kothic/src/drules_info.py"
cp "$OMIM_PATH/protobuf/protobuf-2.6.1-py2.7.egg" "$MAC_RESOURCES/kothic"

# Copy all drules and  resources (required for test environment)
rm -rf $MAC_RESOURCES/drules_proto*
rm -rf $MAC_RESOURCES/resources-*
cp $OMIM_PATH/data/drules_proto_legacy.bin $MAC_RESOURCES/drules_proto.bin
cp -r $OMIM_PATH/data/resources-default/ $MAC_RESOURCES/resources-default/
cp -r $OMIM_PATH/data/resources-6plus_legacy/ $MAC_RESOURCES/resources-6plus/
cp -r $OMIM_PATH/data/resources-6plus_legacy/ $MAC_RESOURCES/resources-6plus/
cp -r $OMIM_PATH/data/resources-ldpi_legacy/ $MAC_RESOURCES/resources-ldpi/
cp -r $OMIM_PATH/data/resources-mdpi_legacy/ $MAC_RESOURCES/resources-mdpi/
cp -r $OMIM_PATH/data/resources-hdpi_legacy/ $MAC_RESOURCES/resources-hdpi/
cp -r $OMIM_PATH/data/resources-xhdpi_legacy/ $MAC_RESOURCES/resources-xhdpi/
cp -r $OMIM_PATH/data/resources-xxhdpi_legacy/ $MAC_RESOURCES/resources-xxhdpi/
cp $OMIM_PATH/data/WorldCoasts_migrate.mwm $MAC_RESOURCES/WorldCoasts_migrate.mwm
cp $OMIM_PATH/data/countries.txt $MAC_RESOURCES/countries.txt
cp $OMIM_PATH/data/countries_migrate.txt $MAC_RESOURCES/countries_migrate.txt
cp $OMIM_PATH/data/packed_polygons.bin $MAC_RESOURCES/packed_polygons.bin
cp $OMIM_PATH/data/packed_polygons_migrate.bin $MAC_RESOURCES/packed_polygons_migrate.bin

# Build DMG image
rm -rf "$BUILD_PATH/deploy"
mkdir "$BUILD_PATH/deploy"
cp -r "$RELEASE_PATH/MAPS.ME.Designer.app" "$BUILD_PATH/deploy/MAPS.ME.Designer.app"
cp -r "$DATA_PATH/styles" "$BUILD_PATH/deploy/styles"

DMG_NAME=MAPS.ME.Designer.$APP_VERSION
hdiutil create -size 640m -volname $DMG_NAME -srcfolder "$BUILD_PATH/deploy" -ov -format UDZO "$BUILD_PATH/$DMG_NAME.dmg"
