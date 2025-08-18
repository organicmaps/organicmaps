#!/usr/bin/env bash

BUILD_PATH="$(dirname "$0")"
OMIM_PATH="$(cd "${OMIM_PATH:-${BUILD_PATH}/../../../..}"; pwd)"

echo "Cleaning.."
rm "${OMIM_PATH}/.dockerignore" 2> /dev/null
mv "${OMIM_PATH}/.dockerignore_" "${OMIM_PATH}/.dockerignore" 2> /dev/null

rm -r "${BUILD_PATH}/storage" 2> /dev/null
