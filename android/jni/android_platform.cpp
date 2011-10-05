#include "android_platform.hpp"
#include "jni_string.h"

#include "../../base/logging.hpp"

void AndroidPlatform::Initialize(JNIEnv * env, jstring apkPath, jstring storagePath)
{
  m_resourcesDir = jni::ToString(env, apkPath);
  m_writableDir = jni::ToString(env, storagePath);
  LOG(LDEBUG, ("Apk path = ", m_resourcesDir));
  LOG(LDEBUG, ("Writable path = ", m_writableDir));
}

AndroidPlatform & GetAndroidPlatform()
{
  static AndroidPlatform platform;
  return platform;
}

Platform & GetPlatform()
{
  return GetAndroidPlatform();
}
