#!/bin/bash
set -e -u

# Prevent python from generating compiled *.pyc files
export PYTHONDONTWRITEBYTECODE=1

DEBUG="${1:-empty}"

MY_PATH="`dirname \"$0\"`"              # relative
MY_PATH="`( cd \"$MY_PATH\" && pwd )`"  # absolutized and normalized

source "$MY_PATH/../autobuild/ndk_helper.sh"
export NDK_ROOT=$(GetNdkRoot)
if [ -z "$NDK_ROOT" ]
then
    echo "Can't find NDK root path"; exit 1 
fi
KERNEL_NAME="$( uname -s )"
if [[ $KERNEL_NAME == 'Darwin' ]]
then
    GLSLC_PATH="$NDK_ROOT/shader-tools/darwin-x86_64/glslc"
elif [[ $KERNEL_NAME == 'Linux' ]]
then
    GLSLC_PATH="$NDK_ROOT/shader-tools/linux-x86_64/glslc"
else
    echo "Unknown kernel"; exit 1
fi

OMIM_PATH="${OMIM_PATH:-$(cd "$(dirname "$0")/../.."; pwd)}"
SHADERS_GENERATOR="$OMIM_PATH/shaders/vulkan_shaders_preprocessor.py"

python3 "$SHADERS_GENERATOR" "$OMIM_PATH/shaders/GL" shader_index.txt programs.hpp program_params.hpp shaders_lib.glsl "$OMIM_PATH/data/vulkan_shaders" "$GLSLC_PATH" "$DEBUG"