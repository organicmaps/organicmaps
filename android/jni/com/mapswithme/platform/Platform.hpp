#pragma once

#include <jni.h>

#include "platform/platform.hpp"


namespace android
{
  class Platform : public ::Platform
  {
  public:
    void Initialize(JNIEnv * env,
                    jstring apkPath, jstring storagePath,
                    jstring tmpPath, jstring obbGooglePath,
                    jstring flavorName, jstring buildType,
                    bool isYota, bool isTablet);

    /// get storage path without ending "/MapsWithMe/"
    string GetStoragePathPrefix() const;
    /// assign storage path (should contain ending "/MapsWithMe/")
    void SetStoragePath(string const & path);

    bool HasAvailableSpaceForWriting(uint64_t size) const;

    static void RunOnGuiThreadImpl(TFunctor const & fn, bool blocking = false);

    static Platform & Instance();
  };
}
