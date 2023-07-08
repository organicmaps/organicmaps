#!/usr/bin/env bash

BUILD_PATH="$(dirname "$0")"
OMIM_PATH="$(cd "${OMIM_PATH:-${BUILD_PATH}/../../../..}"; pwd)"

echo "Build airmaps service.."

mv "${OMIM_PATH}/.dockerignore" "${OMIM_PATH}/.dockerignore_" 2> /dev/null
cp "${BUILD_PATH}/.dockerignore" ${OMIM_PATH}

docker-compose build

rm "${OMIM_PATH}/.dockerignore"
mv "${OMIM_PATH}/.dockerignore_" "${OMIM_PATH}/.dockerignore" 2> /dev/null
