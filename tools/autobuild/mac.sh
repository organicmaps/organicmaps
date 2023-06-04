source build.sh

QMAKE_PATH="/Users/Alex/QtSDK/Desktop/Qt/474/gcc/bin"
SHADOW_DIR="/Developer/omim/omim-mac-release"
MKSPEC="macx-llvm"
QMAKE_PARAMS="CONFIG+=release"

BuildQt "$QMAKE_PATH" "$SHADOW_DIR" "$MKSPEC" "$QMAKE_PARAMS" || echo "ERROR BUILDING PROJECT"
