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
                    jstring apkPath,
                    jstring storagePath,
                    jstring tmpPath,
                    jstring extTmpPath,
                    bool isPro);

    void OnExternalStorageStatusChanged(bool isAvailable);

    /// get storage path without ending "/MapsWithMe/"
    string GetStoragePathPrefix() const;
    /// assign storage path (should contain ending "/MapsWithMe/")
    void SetStoragePath(string const & path);

    bool HasAvailableSpaceForWriting(uint64_t size) const;

    static Platform & Instance();
  };
}
