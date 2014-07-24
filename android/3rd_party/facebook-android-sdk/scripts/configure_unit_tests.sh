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

# this script configures your Android simulator for unit tests
# Note: On Mac OS X, an easy way to generate a MACHINE_UNIQUE_USER_TAG is with the following:
#   system_profiler SPHardwareDataType | grep -i "Serial Number (system):" | awk '{print $4}'

cd $(dirname $0)/..
FB_SDK_ROOT=$(pwd)
FB_SDK_TESTS=$FB_SDK_ROOT/facebook/tests

if [ "$#" -lt 3 ]; then
      echo "Usage: $0 APP_ID APP_SECRET CLIENT_TOKEN [MACHINE_UNIQUE_USER_KEY]"
      echo "  APP_ID                   your unit-testing Facebook application's App ID"
      echo "  APP_SECRET               your unit-testing Facebook application's App Secret"
      echo "  CLIENT_TOKEN             your unit-testing Facebook application's client token"
      echo "  MACHINE_UNIQUE_USER_TAG  optional text used to ensure this machine will use its own set of test users rather than sharing"
      die 'Arguments do not conform to usage'
fi

function write_config_json {
    CONFIG_JSON_FILE="$FB_SDK_TESTS"/assets/config.json

    mkdir -p "$FB_SDK_TESTS"/assets

    # use heredoc syntax to output the json
    cat > "$CONFIG_JSON_FILE" \
<<DELIMIT
{"applicationId":"$1","applicationSecret":"$2","clientToken":"$3","machineUniqueUserTag":"$4"}
DELIMIT
# end heredoc

    echo "wrote unit test config file at $CONFIG_JSON_FILE" 
}

write_config_json $1 $2 $3

