#!/usr/bin/env bash
#
# Downloads metadata from Google Play
#

set -euo pipefail

cd "$(dirname "$0")/../../android"

check_keys() {
    if [[ ! -r "google-play.json" ]]; then
        echo >&2 "Missing $PWD/google-play.json"
        exit 2
    fi
}

check_screenshots() {
    if [[ ! -r "../screenshots/android/en-US/graphics/phone-screenshots/1.jpg" ]]; then
        echo >&2 "Please checkout screenshots to $PWD/../screenshots"
        exit 3
    fi
}

download_metadata() {
    set -x
    ./gradlew bootstrapGoogleRelease

    for locale in src/google/play/listings/??-??; do
        # Fix wrong file extension (.png instead of .jpg)
        for png in $locale/graphics/*-screenshots/*.png; do
            if [[ $(file -b "$png") =~ ^'JPEG ' ]]; then
                echo "Fixing $png extension"
                mv -f "$png" "${png%.png}.jpg"
            fi
        done
        # Copy new files to screenshots repository
        cp -Rpv $locale/graphics ../screenshots/android/$(basename $locale)/
        # Replace original directory with a symlink to screenshots repository
        rm -rf $locale/graphics
        ln -sf ../../../../../../screenshots/android/$(basename $locale)/graphics $locale/graphics
    done

    # Ignore changelogs from all tracks exception production
    mv -f src/google/play/release-notes/en-US/production.txt src/google/play/release-notes/en-US/default.txt
    rm -f src/google/play/release-notes/en-US/alpha.txt src/google/play/release-notes/en-US/beta.txt src/google/play/release-notes/en-US/internal.txt
}

check_keys
check_screenshots
download_metadata
