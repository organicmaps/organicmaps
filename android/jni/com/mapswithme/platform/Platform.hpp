#pragma once

#include <jni.h>

#include "../../../../../platform/platform.hpp"


namespace android
{
  class Platform : public ::Platform
  {
  public:
    void Initialize(JNIEnv * env,
                    jstring apkPath, jstring storagePath,
                    jstring tmpPath, jstring obbGooglePath,
                    bool isPro, bool isYota);

    void OnExternalStorageStatusChanged(bool isAvailable);

    /// get storage path without ending "/MapsWithMe/"
    string GetStoragePathPrefix() const;
    /// assign storage path (should contain ending "/MapsWithMe/")
    void SetStoragePath(string const & path);

    bool HasAvailableSpaceForWriting(uint64_t size) const;

    void RunOnGuiThreadImpl(TFunctor const & fn);

    static Platform & Instance();
  };
}
