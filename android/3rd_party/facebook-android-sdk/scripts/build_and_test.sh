#!/bin/sh
#
# Copyright 2010-present Facebook.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#    http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

function die() {
  echo "$@"
  exit 1
}

# Find enlistment root
cd $(dirname $0) >/dev/null
SDK=$(git rev-parse --show-toplevel) ||
  die "Could not access git"

# Build all samples
cd $SDK/samples
for SAMPLE in *; do
  if [[ -d $SAMPLE ]]; then
    cd $SAMPLE
    ant clean ||
      die "Error running 'ant clean' on sample $SAMPLE"
    ant debug ||
      die "Error running 'ant debug' on sample $SAMPLE"
    cd ..
  fi
done

# Remove any stale test bits, ignore errors here
adb uninstall com.facebook.sdk.tests 2>/dev/null

# Build and run tests
cd $SDK/facebook/tests
ant clean ||
  die "Error running 'ant clean' on facebook sdk"
ant debug install ||
  die "Error running 'ant debug install' on facebook sdk"
ant run-tests ||
  die "Error running 'ant run-tests' on facebook sdk"
