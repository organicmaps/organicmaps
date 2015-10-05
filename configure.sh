#!/bin/sh
# Please run this script to configure the repository after cloning it.

# Stop on the first error.
set -e

PRIVATE_HEADER="private.h"
PRIVATE_PROPERTIES="android/secure.properties"
SAVED_PRIVATE_REPO_FILE=".private_repository_url"
TMP_REPO_DIR=".tmp.private.repo"

if [ ! -f "./omim.pro" ]; then
  echo "Please run this script from the root repository folder."
  exit -1
fi

if [ -f "$SAVED_PRIVATE_REPO_FILE" ]; then
  PRIVATE_REPO=`cat "$SAVED_PRIVATE_REPO_FILE"`
  echo "Using stored private repository URL: $PRIVATE_REPO"
else
  echo "If you are developer from MAPS.ME team, please specify a private repository url here."
  echo "If not [yet :)], then just press Enter."
  echo -n "> "
  read PRIVATE_REPO
  if [ -z "$PRIVATE_REPO" ]; then
    echo "Initializing repository with default values in Open-Source mode."
    echo '
#pragma once

#define ALOHALYTICS_URL ""
#define FLURRY_KEY ""
#define MY_TRACKER_KEY ""
#define PARSE_APPLICATION_ID ""
#define PARSE_CLIENT_KEY ""
#define MWM_GEOLOCATION_SERVER ""
#define OSRM_ONLINE_SERVER_URL ""
#define RESOURCES_METASERVER_URL ""
#define METASERVER_URL ""
#define DEFAULT_URLS_JSON ""
' > "$PRIVATE_HEADER"
    echo '
ext {
  spropStoreFile = "../tools/android/debug.keystore"
  spropStorePassword = "12345678"
  spropKeyAlias = "debug"
  spropKeyPassword = "12345678"
  spropYotaStoreFile = "../tools/android/debug.keystore"
  spropYotaStorePassword = "12345678"
  spropYotaKeyAlias = "debug"
  spropYotaKeyPassword = "12345678"
}
' > "$PRIVATE_PROPERTIES"
    exit
  fi
fi

git clone --depth 1 "$PRIVATE_REPO" "$TMP_REPO_DIR"
if [ $? == 0 ]; then
  echo "Saved private repository url to $SAVED_PRIVATE_REPO_FILE"
  echo "$PRIVATE_REPO" > "$SAVED_PRIVATE_REPO_FILE"
  rm -rf "$TMP_REPO_DIR/.git" "$TMP_REPO_DIR/README.md"
  cp -Rv "$TMP_REPO_DIR"/* .
  rm -rf "$TMP_REPO_DIR"
  echo "Private files have been updated."
fi
