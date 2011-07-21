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

void AndroidPlatform::GetFontNames(FilesList & res) const
{
  res.push_back("01_dejavusans.ttf");
  res.push_back("02_wqy-microhei.ttf");
  res.push_back("03_jomolhari-id-a3d.ttf");
  res.push_back("04_padauk.ttf");
  res.push_back("05_khmeros.ttf");
  res.push_back("06_code2000.ttf");

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

double AndroidPlatform::VisualScale() const
{
  return 1.3;
}

string AndroidPlatform::SkinName() const
{
  return "basic.skn";
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
