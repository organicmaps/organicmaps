#include "platform.h"
#include "jni_string.h"


void AndroidPlatform::Initialize(JNIEnv * env, jobject activity, jstring path)
{
  m_writableDir = m_resourcesDir = jni::ToString(env, path);
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
