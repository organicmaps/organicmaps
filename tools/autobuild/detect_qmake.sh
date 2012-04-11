# Add your path into this array
KNOWN_QMAKE_PATHS=( \
  /Users/Alex/QtSDK/Desktop/Qt/4.8.1/gcc/bin \
  /Users/siarheirachytski/QtSDK/Desktop/Qt/474/gcc/bin \
)

# Prints path to directory with found qmake binary or prints nothing if not found
# Returns 1 in case of not found and 0 in case of success
PrintQmakePath() {
  local QMAKE_PATH
  QMAKE_PATH=$(which qmake)
  if [ $? -ne 0 ]; then
    # qmake binary is not in the path, look for it in the given array
    for path in "${KNOWN_QMAKE_PATHS[@]}"; do
      if [ -f "${path}/qmake" ]; then
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
