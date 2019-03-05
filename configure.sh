#!/bin/sh
# Please run this script to configure the repository after cloning it.

# Stop on the first error.
set -e -u

BASE_PATH=`dirname "$0"`

PRIVATE_HEADER="$BASE_PATH/private.h"
DEFAULT_PRIVATE_HEADER="$BASE_PATH/private_default.h"
PRIVATE_CAR_MODEL_COEFS="$BASE_PATH/routing_common/car_model_coefs.hpp"
DEFAULT_PRIVATE_CAR_MODEL_COEFS="$BASE_PATH/routing_common/car_model_coefs_default.hpp"

PRIVATE_PROPERTIES="$BASE_PATH/android/secure.properties"
PRIVATE_FABRIC_PROPERTIES="$BASE_PATH/android/fabric.properties"
PRIVATE_PUSHWOOSH_PROPERTIES="$BASE_PATH/android/pushwoosh.properties"
PRIVATE_LIBNOTIFY_PROPERTIES="$BASE_PATH/android/libnotify.properties"
PRIVATE_NETWORK_CONFIG="$BASE_PATH/android/res/xml/network_security_config.xml"
SAVED_PRIVATE_REPO_FILE="$BASE_PATH/.private_repository_url"
TMP_REPO_DIR="$BASE_PATH/.tmp.private.repo"

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
    cat "$DEFAULT_PRIVATE_HEADER" > "$PRIVATE_HEADER"
    cat "$DEFAULT_PRIVATE_CAR_MODEL_COEFS" > "$PRIVATE_CAR_MODEL_COEFS"
    echo 'ext {
  spropStoreFile = "../tools/android/debug.keystore"
  spropStorePassword = "12345678"
  spropKeyAlias = "debug"
  spropKeyPassword = "12345678"
}
' > "$PRIVATE_PROPERTIES"

    echo 'appId=XXXXX
projectId=00000000
' > "$PRIVATE_LIBNOTIFY_PROPERTIES"

    echo 'apiSecret=0000000000000000000000000000000000000000000000000000000000000000
apiKey=0000000000000000000000000000000000000000
' > "$PRIVATE_FABRIC_PROPERTIES"
    echo 'pwAppId=XXXXX
pwProjectId=A123456789012
' > "$PRIVATE_PUSHWOOSH_PROPERTIES"
    echo '<?xml version="1.0" encoding="utf-8"?>
<network-security-config/>
' > "$PRIVATE_NETWORK_CONFIG"
    exit
  fi
fi

if git clone --depth 1 "$PRIVATE_REPO" "$TMP_REPO_DIR"; then
  echo "Saved private repository url to $SAVED_PRIVATE_REPO_FILE"
  echo "$PRIVATE_REPO" > "$SAVED_PRIVATE_REPO_FILE"
  rm -rf "$TMP_REPO_DIR/.git" "$TMP_REPO_DIR/README.md"
  cp -Rv "$TMP_REPO_DIR"/* "$BASE_PATH"
  rm -rf "$TMP_REPO_DIR"
  echo "Private files have been updated."
fi
