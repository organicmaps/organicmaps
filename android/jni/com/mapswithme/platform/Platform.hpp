#pragma once

#include <jni.h>

#include "MethodRef.hpp"

#include "platform/platform.hpp"

namespace android
{
  class Platform : public ::Platform
  {
  public:
    Platform();
    void Initialize(JNIEnv * env,
                    jstring apkPath, jstring storagePath,
                    jstring tmpPath, jstring obbGooglePath,
                    jstring flavorName, jstring buildType,
                    bool isYota, bool isTablet);

    void InitAppMethodRefs(JNIEnv * env, jobject appObject);
    void CallNativeFunctor(jlong functionPointer);

    void OnExternalStorageStatusChanged(bool isAvailable);

    /// get storage path without ending "/MapsWithMe/"
    string GetStoragePathPrefix() const;
    /// assign storage path (should contain ending "/MapsWithMe/")
    void SetStoragePath(string const & path);

    bool HasAvailableSpaceForWriting(uint64_t size) const;
    void RunOnGuiThread(TFunctor const & fn);

    static Platform & Instance();

  private:
    MethodRef m_runOnUI;
  };
}
