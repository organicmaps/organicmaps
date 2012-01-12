#pragma once

#include <jni.h>

#include "../../../../../platform/platform.hpp"

namespace android
{
  class Platform : public ::Platform
  {
    /// External storage path for temporary files, used when external storage is available
    string m_externalTmpPath;
    /// The same but in device's internal memory (it's usually much smaller)
    string m_localTmpPath;

  public:
    ~Platform();
    void Initialize(JNIEnv * env,
                    jint densityDpi,
                    jint screenWidth,
                    jint screenHeight,
                    jstring apkPath,
                    jstring storagePath,
                    jstring tmpPath,
                    jstring extTmpPath,
                    jstring settingsPath);

    void OnExternalStorageStatusChanged(bool isAvailable);

    static Platform & Instance();
  };
}
