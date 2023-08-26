#!/usr/bin/env bash

ANDROID_LISTINGS=android/app/src/main/fdroid/play/listings
ANDROID_NOTES=$ANDROID_LISTINGS/en-US/release-notes.txt
IOS_METADATA=iphone/metadata
IOS_NOTES=$IOS_METADATA/en-US/release_notes.txt

find $ANDROID_LISTINGS -name release-notes.txt -exec rsync -a $ANDROID_NOTES {} \;
find $IOS_METADATA -name release_notes.txt -exec rsync -a $IOS_NOTES {} \;
