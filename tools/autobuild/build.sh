set -e -x

LOCAL_DIRNAME="${PWD}/$(dirname "$0")"

source "$LOCAL_DIRNAME/detect_qmake.sh"

# 1st param: shadow directory path
# 2nd param: mkspec
# 3rd param: additional qmake parameters
BuildQt() {
  (
    # set qmake path
    PATH="$(PrintQmakePath):$PATH" || ( echo "ERROR: qmake was not found, please add it to your PATH or into the tools/autobuild/detect_qmake.sh"; exit 1 )
    SHADOW_DIR="$1"
    MKSPEC="$2"
    QMAKE_PARAMS="$3"

    mkdir -p "$SHADOW_DIR"
    cd "$SHADOW_DIR"
    qmake -r "$QMAKE_PARAMS" -spec "$MKSPEC" "../omim/omim.pro"
#    make clean > /dev/null || true
    make
  )
}
