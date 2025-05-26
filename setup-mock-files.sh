#!/bin/bash

# Create directories if they do not exist
mkdir -p $GITHUB_WORKSPACE/organicmaps/data/bookmarks
mkdir -p $GITHUB_WORKSPACE/organicmaps/data/test_data
mkdir -p /tmp

# Create empty mock files
touch $GITHUB_WORKSPACE/organicmaps/data/power_manager_config
touch $GITHUB_WORKSPACE/organicmaps/data/bookmarks/cat1.kmb
touch $GITHUB_WORKSPACE/organicmaps/data/bookmarks/cat2.kmb
touch $GITHUB_WORKSPACE/organicmaps/data/bookmarks/cat3.kmb
touch $GITHUB_WORKSPACE/organicmaps/data/bookmarks/Bookmarks.kmb
touch $GITHUB_WORKSPACE/organicmaps/data/bookmarks/xxx.kmb
touch $GITHUB_WORKSPACE/organicmaps/data/test_data/broken_bookmarks.kmb.test
touch $GITHUB_WORKSPACE/organicmaps/data/gpstrack_test.bin
touch /tmp/tmp.xml
touch /tmp/Some\ random\ route.kml
touch /tmp/new.kml
touch /tmp/OrganicMaps_1.kml
touch /tmp/OrganicMaps_2.kml