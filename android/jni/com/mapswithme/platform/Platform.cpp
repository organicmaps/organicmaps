#include "Platform.hpp"

#include "../core/jni_string.hpp"

#include "../../../../../base/logging.hpp"

namespace android
{
  void Platform::Initialize(JNIEnv * env, jstring apkPath, jstring storagePath,
      jstring tmpPath, jstring settingsPath)
  {
    m_resourcesDir = jni::ToString(env, apkPath);
    m_writableDir = jni::ToString(env, storagePath);
    m_tmpDir = jni::ToString(env, tmpPath);
    m_settingsDir = jni::ToString(env, settingsPath);

    LOG(LDEBUG, ("Apk path = ", m_resourcesDir));
    LOG(LDEBUG, ("Writable path = ", m_writableDir));
  }

  Platform & Platform::Instance()
  {
    static Platform platform;
    return platform;
  }
}

Platform & GetPlatform()
{
  return android::Platform::Instance();
}
