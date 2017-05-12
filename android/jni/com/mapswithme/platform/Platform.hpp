#pragma once

#include <jni.h>

#include "platform/platform.hpp"

namespace android
{
  class Platform : public ::Platform
  {
  public:
    void Initialize(JNIEnv * env,
                    jobject functorProcessObject,
                    jstring apkPath, jstring storagePath,
                    jstring tmpPath, jstring obbGooglePath,
                    jstring flavorName, jstring buildType,
                    bool isTablet);

    void ProcessFunctor(jlong functionPointer);

    void OnExternalStorageStatusChanged(bool isAvailable);

    /// get storage path without ending "/MapsWithMe/"
    std::string GetStoragePathPrefix() const;
    /// assign storage path (should contain ending "/MapsWithMe/")
    void SetWritableDir(std::string const & dir);
    void SetSettingsDir(std::string const & dir);

    bool HasAvailableSpaceForWriting(uint64_t size) const;
    void RunOnGuiThread(TFunctor const & fn);

    void SendPushWooshTag(std::string const & tag, std::vector<std::string> const & values);
    void SendMarketingEvent(std::string const & tag, std::map<std::string, std::string> const & params);

    static Platform & Instance();

  private:
    jobject m_functorProcessObject;
    jmethodID m_functorProcessMethod;
    jmethodID m_sendPushWooshTagsMethod;
    jmethodID m_myTrackerTrackMethod;
  };
} // namespace android
