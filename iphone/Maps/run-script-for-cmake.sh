#!/bin/bash
# This script builds C++ core libs and inserts some private variables.
# Should be run from Run Script phase in target's settings.

LOWERED_CONFIG=`echo $CONFIGURATION | tr [A-Z] [a-z]`
CONF="debug"
if [[ "$LOWERED_CONFIG" == *release* || "$LOWERED_CONFIG" == *production* || "$LOWERED_CONFIG" == *adhoc* ]]; then
  CONF="release"
fi

# Respect "Build for active arch only" project setting.
if [[ "$ONLY_ACTIVE_ARCH" == YES ]]; then
  if [[ ! -z $CURRENT_ARCH ]]; then
    VALID_ARCHS=${ARCHS[0]}
  fi
fi

echo "Building $CONF configuration"
bash "$SRCROOT/../../tools/autobuild/ios_cmake.sh" $CONF
