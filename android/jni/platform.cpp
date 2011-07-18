#include "platform.h"
#include "jni_string.h"

#include "../../base/logging.hpp"

#include "../../coding/zip_reader.hpp"


void AndroidPlatform::Initialize(JNIEnv * env, jobject activity, jstring apkPath, jstring storagePath)
{
  m_resourcesDir = jni::ToString(env, apkPath);
  m_writableDir = jni::ToString(env, storagePath);
  LOG(LDEBUG, ("Apk path = ", m_resourcesDir));
  LOG(LDEBUG, ("Writable path = ", m_writableDir));
}

ModelReader * AndroidPlatform::GetReader(string const & file) const
{
  if (IsFileExists(m_writableDir + file))
    return BasePlatformImpl::GetReader(file);
  else
    return new ZipFileReader(m_resourcesDir, "assets/" + file);
}

bool AndroidPlatform::IsMultiSampled() const
{
  return false;
}

void AndroidPlatform::GetFontNames(FilesList & res) const
{
  res.push_back("01_dejavusans.ttf");
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
