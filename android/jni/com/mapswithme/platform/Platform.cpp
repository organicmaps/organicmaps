#include "Platform.hpp"

#include "../core/jni_string.hpp"

#include "../../../../../base/logging.hpp"

namespace android
{
  void Platform::Initialize(JNIEnv * env, jstring apkPath, jstring storagePath)
  {
    m_resourcesDir = jni::ToString(env, apkPath);
    m_writableDir = jni::ToString(env, storagePath);
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
