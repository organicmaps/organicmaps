#!/bin/bash
set -euo pipefail

source "${OMIM_ROOT}/tools/autobuild/detect_cmake.sh"

sperr_build_dir="${DERIVED_FILE_DIR}/sperr"
sperr_archs="${ARCHS// /;}"
sperr_options=(
  -DCMAKE_BUILD_TYPE="${CONFIGURATION}"
  -DCMAKE_OSX_SYSROOT="${SDKROOT}"
  -DCMAKE_OSX_ARCHITECTURES="${sperr_archs}"
  -DCMAKE_CXX_VISIBILITY_PRESET=hidden
  -DCMAKE_VISIBILITY_INLINES_HIDDEN=ON
)
case "${PLATFORM_NAME}" in
  iphone*)
    [[ "${ARCHS}" != *x86_64* ]] || { echo "SPERR iOS requires arm64" >&2; exit 1; }
    sperr_options+=(-DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_DEPLOYMENT_TARGET="${IPHONEOS_DEPLOYMENT_TARGET}") ;;
  macosx) sperr_options+=(-DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET}") ;;
esac

"${CMAKE}" -S "${OMIM_ROOT}/3party/sperr" -B "${sperr_build_dir}" "${sperr_options[@]}"
"${CMAKE}" --build "${sperr_build_dir}" --target omim_sperr --parallel "${NCPU:-4}"
