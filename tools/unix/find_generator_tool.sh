#!/bin/bash
#####################################
# Locates generator_tool executable #
#####################################

# Set GENERATOR_TOOL to explicitly use one
# Or BUILD_PATH to point to a build directory

OMIM_PATH="${OMIM_PATH:-$(cd "$(dirname "$0")/../.."; pwd)}"

if [ -z "${GENERATOR_TOOL-}" -o ! -x "${GENERATOR_TOOL-}" ]; then
  IT_PATHS_ARRAY=()
  for i in "${BUILD_PATH-}" "$OMIM_PATH" "$OMIM_PATH/.."/*omim*elease* "$OMIM_PATH/.."/*omim*ebug; do
    IT_PATHS_ARRAY+=("$i/generator_tool")
  done

  if [ -d "$OMIM_PATH/../omim-xcode-build" ]; then
    IT_PATHS_ARRAY+=("$OMIM_PATH/../omim-xcode-build/Release" "$OMIM_PATH/../omim-xcode-build/Debug")
  fi

  for i in "${BUILD_PATH:+$BUILD_PATH/generator_tool}" "${IT_PATHS_ARRAY[@]}"; do
    if [ -x "$i" ]; then
      GENERATOR_TOOL="$i"
      break
    fi
  done
fi

[ -z "${GENERATOR_TOOL-}" -o ! -x "${GENERATOR_TOOL-}" ] && fail "No generator_tool found in ${IT_PATHS_ARRAY[*]-${GENERATOR_TOOL-}}"
echo "Using tool: $GENERATOR_TOOL"
