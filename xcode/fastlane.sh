#!/usr/bin/env bash
#
# A convenient wrapper for fastlane
#

set -euo pipefail

cd "$(dirname "$0")"

check_keys() {
    if [[ ! -r "keys/appstore.json" ]]; then
        echo >&2 "Missing keys/"
        exit 2
    fi
}

check_screenshots() {
    if [[ ! -r ../screenshots/ios/en-US/0_APP_IPHONE_65_0.png ]]; then
        echo >&2 "Please checkout screenshots to ../screenshots"
        exit 3
    fi
}

run_fastlane() {
    export FASTLANE_SKIP_UPDATE_CHECK=true
    echo fastlane $@
    fastlane $@
}

download_metadata() {
    check_keys
    run_fastlane deliver download_metadata \
        --force
    rm -rf ../iphone/metadata/review_information
    rm -f ../iphone/metadata/*/apple_tv_privacy_policy.txt
    exit 0
}

download_screenshots() {
    check_keys
    check_screenshots
    run_fastlane deliver download_screenshots \
        --force
    echo >&2
    echo >&2 "(!) Please don't forget to commit changes at ../screenshots"
    echo >&2
    exit 0
}

upload_metadata() {
    check_keys
    run_fastlane deliver \
        --force \
        --skip_binary_upload=true \
        --skip_app_version_update=true \
        --skip_screenshots \
        --precheck_include_in_app_purchases=false \
        --automatic_release=false
}

upload_screenshots() {
    check_keys
    check_screenshots
    run_fastlane deliver \
        --force \
        --skip_binary_upload=true \
        --skip_app_version_update=true \
        --skip_metadata \
        --overwrite_screenshots=true \
        --precheck_include_in_app_purchases=false \
        --automatic_release=false
}

upload_testflight() {
    check_keys
    run_fastlane upload_testflight
}

case ${1:-default} in
download_metadata)
    download_metadata
    ;;
download_screenshots)
    download_screenshots
    ;;
upload_metadata)
    upload_metadata
    ;;
upload_screenshots)
    upload_screenshots
    ;;
upload_testflight)
    upload_testflight
    ;;
*)
    echo >&2 "Usage:"
    echo >&2 "$0 download_metadata     # Download metadata from AppStore"
    echo >&2 "$0 download_screenshots  # Download screenshots from AppStore"
    echo >&2 "$0 upload_metadata       # Download metadata to AppStore"
    echo >&2 "$0 upload_screenshots    # Download screenshots to AppStore"
    echo >&2 "$0 upload_testflight     # Build and upload new beta version to TestFlight"
    exit 1
    ;;
esac
