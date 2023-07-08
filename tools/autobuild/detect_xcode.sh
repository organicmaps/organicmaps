# Prints path to directory with iOS SDK
# Pameter is configuration name: <debug|release|production|simulator>
# Returns 1 in case of not found and 0 in case of success
PrintIOSSDKPath() {
  if [[ $1 == *simulator* ]]; then
    XCODE_SDK_PATH=`xcodebuild -sdk iphonesimulator -version Path`
  else
    XCODE_SDK_PATH=`xcodebuild -sdk iphoneos -version Path`
  fi
  if [ -z "$XCODE_SDK_PATH" ]; then
    return 1
  else
    echo -n "$XCODE_SDK_PATH"
  fi
  return 0
}
