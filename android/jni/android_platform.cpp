#include "android_platform.hpp"
#include "jni_string.h"

#include "../../base/logging.hpp"

#include "../../coding/zip_reader.hpp"

void AndroidPlatform::Initialize(JNIEnv * env, jstring apkPath, jstring storagePath)
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
  { // paths from GetFilesInDir will already contain "assets/"
    if (file.find("assets/") != string::npos)
      return new ZipFileReader(m_resourcesDir, file);
    else
      return new ZipFileReader(m_resourcesDir, "assets/" + file);
  }
}

void AndroidPlatform::GetFilesInDir(string const & directory, string const & mask, FilesList & res) const
{
  if (ZipFileReader::IsZip(directory))
  { // Get files list inside zip file
    res = ZipFileReader::FilesList(directory);
    // filter out according to the mask
    // @TODO we don't support wildcards at the moment
    string fixedMask = mask;
    if (fixedMask.size() && fixedMask[0] == '*')
      fixedMask.erase(0, 1);
    for (FilesList::iterator it = res.begin(); it != res.end();)
    {
      if (it->find(fixedMask) == string::npos)
        it = res.erase(it);
      else
        ++it;
    }
  }
  else
    BasePlatformImpl::GetFilesInDir(directory, mask, res);
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
