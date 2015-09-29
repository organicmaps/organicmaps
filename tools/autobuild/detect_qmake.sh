# Add your path into this array
KNOWN_QMAKE_PATHS=( \
  /Developer/Tools/qmake \
  ~/Developer/Qt-4.8.4/bin/qmake \
  ~/Developer/Qt/5.1.0/clang_64/bin/qmake \
  ~/Developer/Qt/5.1.1/clang_64/bin/qmake \
  ~/Developer/Qt/5.2.0/clang_64/bin/qmake \
  ~/Developer/Qt/5.2.1/clang_64/bin/qmake \
  ~/Qt/5.2.1/clang_64/bin/qmake \
  ~/Developer/Qt/5.3/clang_64/bin/qmake \
  ~/Qt/5.3/clang_64/bin/qmake \
  ~/Qt/5.4/clang_64/bin/qmake \
  ~/Qt/5.5/clang_64/bin/qmake \
  ~/Qt5.3.0/5.3/clang_64/bin/qmake \
  ~/Developer/Qt/5.4/clang_64/bin/qmake \
  ~/Developer/Qt/5.5/clang_64/bin/qmake \
  /usr/local/opt/qt5/bin/qmake \
  /cygdrive/c/Qt/5.5/msvc2013_64/bin/qmake.exe \
)

# Prints path to directory with found qmake binary or prints nothing if not found
# Returns 1 in case of not found and 0 in case of success
PrintQmakePath() {
  local QMAKE_PATH
  QMAKE_PATH=$(which qmake)
  if [ $? -ne 0 ]; then
    # qmake binary is not in the path, look for it in the given array
    for path in "${KNOWN_QMAKE_PATHS[@]}"; do
      if [ -f "${path}" ]; then
        echo "${path}"
        return 0
      fi
    done
  else
    echo "${QMAKE_PATH}"
    return 0
  fi
  # Not found
  return 1
}
