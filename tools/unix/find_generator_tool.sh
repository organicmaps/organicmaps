#!/bin/bash
#####################################
# Locates generator_tool executable #
#####################################

# Set GENERATOR_TOOL to explicitly use one
# Or BUILD_PATH to point to a build directory

OMIM_PATH="${OMIM_PATH:-$(cd "$(dirname "$0")/../.."; pwd)}"

if [ -z "${GENERATOR_TOOL-}" ]; then
  IT_PATHS_ARRAY=()
  for i in ${BUILD_PATH-} $OMIM_PATH $OMIM_PATH/../*omim*elease* $OMIM_PATH/../*omim*ebug; do
    if [ -d "$i/out" ]; then
      IT_PATHS_ARRAY+=("$i/out/release/generator_tool" "$i/out/debug/generator_tool")
    fi
  done

  for i in "${BUILD_PATH:+$BUILD_PATH/generator_tool}" ${IT_PATHS_ARRAY[@]}; do
    if [ -x "$i" ]; then
      GENERATOR_TOOL="$i"
      break
    fi
  done
fi

[ -z ${GENERATOR_TOOL-} -o ! -x "${GENERATOR_TOOL-}" ] && fail "No generator_tool found in ${IT_PATHS_ARRAY[*]}"
echo "Using tool: $GENERATOR_TOOL"
