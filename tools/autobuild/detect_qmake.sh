#!/bin/bash
set -e -u

# If QMAKE variable is set, use it
[ -n "${QMAKE-}" -a -x "${QMAKE-}" ] && return 0

# Add your path into this array
KNOWN_QMAKE_PATHS=( \
  /Developer/Tools/qmake \
  /usr/local/opt/qt5/bin/qmake \
  /usr/local/opt/qt@5.?/bin/qmake \
  ~/Developer/Qt/5.?/clang_64/bin/qmake \
  ~/Qt/5.?/clang_64/bin/qmake \
  ~/Qt5.?.0/5.?/clang_64/bin/qmake \
  /cygdrive/c/Qt/5.?/msvc2013_64/bin/qmake.exe \
)

# Prints path to directory with found qmake binary or prints nothing if not found
# Returns 1 in case of not found and 0 in case of success
if QMAKE="$(which qmake-qt5)"; then
  return 0
elif QMAKE="$(which qmake)"; then
  return 0
else
  # qmake binary is not in the path, look for it in the given array
  for path in "${KNOWN_QMAKE_PATHS[@]}"; do
    if [ -x "$path" ]; then
      QMAKE="$path"
      return 0
    fi
  done
fi

# Not found
echo "ERROR: qmake was not found, please add it to your PATH or into the tools/autobuild/detect_qmake.sh"
exit 1
