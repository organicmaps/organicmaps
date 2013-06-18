# Add your paths into these arrays
KNOWN_IOS_SDK_PATHS=( \
  /Applications/XCode5-DP.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS7.0.sdk \
  /Applications/XCode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS5.1.sdk \
  /Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS5.0.sdk \
  /Applications/XCode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS6.0.sdk \
  /Applications/XCode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS6.1.sdk
)

KNOWN_IOS_SDK_SIMULATOR_PATHS=( \
  /Applications/Xcode5-DP.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator7.0.sdk \
  /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator5.1.sdk \
  /Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator5.0.sdk \
  /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator6.0.sdk \
  /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator6.1.sdk
)


# Prints path to directory with iOS SDK
# Pameter is configuration name: <debug|release|production|simulator>
# Returns 1 in case of not found and 0 in case of success
PrintIOSSDKPath() {
  PATHS_ARRAY="${KNOWN_IOS_SDK_PATHS[@]}"
  if [[ $1 == *simulator* ]]; then
    for path in "${KNOWN_IOS_SDK_SIMULATOR_PATHS[@]}"; do
      if [ -d "${path}" ]; then
        echo "${path}"
        return 0
      fi
    done
  else
    for path in "${KNOWN_IOS_SDK_PATHS[@]}"; do
      if [ -d "${path}" ]; then
        echo "${path}"
        return 0
      fi
    done
  fi
  # Not found
  return 1
}
