#!/usr/bin/env bash

BUILD_PATH="$(dirname "$0")"

echo "Creating storage.."
mkdir -p "${BUILD_PATH}/storage/tests/coasts"
mkdir -p "${BUILD_PATH}/storage/tests/maps/open_source"
mkdir -p "${BUILD_PATH}/storage/tests/planet_regular"

chown -R www-data:www-data "${BUILD_PATH}/storage/tests"
