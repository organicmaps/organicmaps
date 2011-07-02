#include "platform.h"
#include "jni_string.h"

#include "../../base/logging.hpp"


void AndroidPlatform::Initialize(JNIEnv * env, jobject activity, jstring apkPath, jstring storagePath)
{
  m_resourcesDir = jni::ToString(env, apkPath);
  m_writableDir = jni::ToString(env, storagePath);
  LOG(LDEBUG, ("Apk path = ", m_resourcesDir));
  LOG(LDEBUG, ("Writable path = ", m_writableDir));
}

void AndroidPlatform::GetFontNames(FilesList & res) const
{
  /// @todo Need to make refactoring of yg fonts
}

int AndroidPlatform::CpuCores() const
{
  return 1;
}

string AndroidPlatform::DeviceID() const
{
  return "Android";
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
