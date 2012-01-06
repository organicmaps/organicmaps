#include "Platform.hpp"

#include "../core/jni_string.hpp"

#include "../../../../../base/logging.hpp"

namespace android
{
  void Platform::Initialize(JNIEnv * env, jstring apkPath, jstring storagePath,
      jstring tmpPath, jstring extTmpPath, jstring settingsPath)
  {
    m_resourcesDir = jni::ToString(env, apkPath);
    m_writableDir = jni::ToString(env, storagePath);
    m_settingsDir = jni::ToString(env, settingsPath);

    m_localTmpPath = jni::ToString(env, tmpPath);
    m_externalTmpPath = jni::ToString(env, extTmpPath);
    // By default use external temporary folder
    m_tmpDir = m_externalTmpPath;

    LOG(LDEBUG, ("Apk path = ", m_resourcesDir));
    LOG(LDEBUG, ("Writable path = ", m_writableDir));
  }

  void Platform::OnExternalStorageStatusChanged(bool isAvailable)
  {
    if (isAvailable)
      m_tmpDir = m_externalTmpPath;
    else
      m_tmpDir = m_localTmpPath;
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
