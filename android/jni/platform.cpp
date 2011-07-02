#include "platform.h"
#include "jni_string.h"

#include "../../base/logging.hpp"


void AndroidPlatform::Initialize(JNIEnv * env, jobject activity, jstring path)
{
  m_writableDir = m_resourcesDir = jni::ToString(env, path);

  LOG(LDEBUG, ("Resource path = ", m_resourcesDir));
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
