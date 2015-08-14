set -e -u

MY_PATH="`dirname \"$0\"`"              # relative
MY_PATH="`( cd \"$MY_PATH\" && pwd )`"  # absolutized and normalized

# Echoes found NDK root path or nothing if not found
# return 1 on error and 0 on success
GetNdkRoot()
{
  local FILENAME="$MY_PATH/../../android/local.properties"
  while read line
  do
    if [[ "${line:0:7}" == "ndk.dir" ]]; then
      echo "${line:8}"
      return 0
    fi
  done < $FILENAME
  return 1
}

GetNdkHost()
{
  if [[ "${OSTYPE:0:5}" == "linux" ]]; then
    echo "linux-x86_64"
    return 0
  fi
  if [[ "${OSTYPE:0:6}" == "darwin" ]]; then
    echo "darwin-x86_64"
    return 0
  fi
  if [[ "$OSTYPE" == "cygwin" ]]; then
    echo windows-x86_64
    return 0
  fi
  echo "ERROR: Can't detect your host OS"
  return 1
}
