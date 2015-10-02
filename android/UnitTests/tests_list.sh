#!/bin/bash

# This file contains a list of test which will be built for android.
# If you want you can leave a subset of tests here for developent purpose

# @todo this list of test paths should be used in sh scripts and in C++ as well. 
# The main point is you need to add a test to UnitTests project only once and here.

MY_PATH=$(dirname "$0")                 # relative
MY_PATH="`( cd \"$MY_PATH\" && pwd )`"  # absolutized and normalized

declare -r TESTS_LIST=($MY_PATH/../../routing/routing_integration_tests/ $MY_PATH/../../indexer/indexer_tests/)
