#!/usr/bin/env bash
# Archives and uploads the release into the AppStore.
# Assumes that Xcode is signed in to the team, and that the distribution certificate and the
# "NonGPS AppStore" and "NonGPS Widget AppStore" profiles are installed.

set -euxo pipefail

SCRIPT_DIR="$( cd "$(dirname "$0")" && pwd -P )"

IOS_BUILD=$($SCRIPT_DIR/../tools/unix/version.sh ios_build)
IOS_VERSION=$($SCRIPT_DIR/../tools/unix/version.sh ios_version)

BUILD_DIR="$SCRIPT_DIR/build"
mkdir -p "$BUILD_DIR"
ARCHIVE_PATH="$BUILD_DIR/OM-$IOS_VERSION-$IOS_BUILD.xcarchive"
IPA_PATH="$BUILD_DIR"
rm -rf "$ARCHIVE_PATH"


# Build release archive.
xcodebuild archive \
    -workspace "$SCRIPT_DIR/../xcode/omim.xcworkspace" \
    -configuration Release \
    -scheme OMaps \
    -destination generic/platform=iOS \
    -archivePath "$ARCHIVE_PATH" \
    -allowProvisioningUpdates \
    MARKETING_VERSION="$IOS_VERSION" \
    CURRENT_PROJECT_VERSION="$IOS_BUILD"


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
  <string>app-store-connect</string>
  <key>teamID</key>
  <string>VTWF79TU37</string>
  <key>signingStyle</key>
  <string>manual</string>
  <key>signingCertificate</key>
  <string>iOS Distribution</string>
  <key>provisioningProfiles</key>
  <dict>
    <key>com.non-gps</key>
    <string>NonGPS AppStore</string>
    <key>com.non-gps.widgetextension</key>
    <string>NonGPS Widget AppStore</string>
  </dict>
</dict>
</plist>
EOM

# Upload build to the AppStore.
xcodebuild -exportArchive \
    -archivePath "$ARCHIVE_PATH" \
    -exportOptionsPlist "$PLIST"


echo "Build was successfully uploaded! Please don't forget to tag it with release notes using:"
TAG="$IOS_VERSION-$IOS_BUILD-ios"
echo "git tag -a $TAG"
echo "git push origin $TAG"
