#!/usr/bin/env bash
# Archives and uploads the release into the AppStore.
# Assumes that all certificates and CarPlay provisioning profiles are properly set up in XCode.

set -euxo pipefail

SCRIPT_DIR="$( cd "$(dirname "$0")" && pwd -P )"
cd "$SCRIPT_DIR/.."
./configure.sh git@github.com:organicmaps/organicmaps-keys
cd "$SCRIPT_DIR"

# Generate version numbers.
DATE_OF_LAST_COMMIT=$(git log -1 --date=format:%Y-%m-%d --pretty=format:%cd)
NUMBER_OF_COMMITS_ON_THAT_DAY=$(git rev-list --count --after="${DATE_OF_LAST_COMMIT}T00:00:00" HEAD)
# Replace '-' with '.'
IOS_VERSION="${DATE_OF_LAST_COMMIT//-/.}"

BUILD_DIR="$SCRIPT_DIR/build"
mkdir -p "$BUILD_DIR"
ARCHIVE_PATH="$BUILD_DIR/OM-$IOS_VERSION-$NUMBER_OF_COMMITS_ON_THAT_DAY.xcarchive"
IPA_PATH="$BUILD_DIR"
rm -rf "$ARCHIVE_PATH"


# Build release archive.
xcodebuild archive \
    -workspace "$SCRIPT_DIR/../xcode/omim.xcworkspace" \
    -configuration Release \
    -scheme OMaps \
    -archivePath "$ARCHIVE_PATH" \
    MARKETING_VERSION="$IOS_VERSION" \
    CURRENT_PROJECT_VERSION="$NUMBER_OF_COMMITS_ON_THAT_DAY"


# Create a plist with upload options.
PLIST="$BUILD_DIR/export.plist"
cat > "$PLIST" <<EOM
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>destination</key>
  <string>upload</string>
  <key>method</key>
  <string>app-store</string>
  <key>teamID</key>
  <string>9Z6432XD7L</string>
  <key>provisioningProfiles</key>
  <dict>
    <key>app.organicmaps</key>
    <string>CarPlay AppStore</string>
  </dict>
</dict>
</plist>
EOM

# Upload build to the AppStore.
xcodebuild -exportArchive \
    -archivePath "$ARCHIVE_PATH" \
    -exportOptionsPlist "$PLIST"


echo "Build was successfully uploaded! Please don't forget to tag it with release notes using:"
TAG="$IOS_VERSION-$NUMBER_OF_COMMITS_ON_THAT_DAY-ios"
echo "git tag -m $TAG"
echo "git push origin $TAG"
