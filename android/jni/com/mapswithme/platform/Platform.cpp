#include "Platform.hpp"

#include "../core/jni_string.hpp"

#include "../../../../../base/logging.hpp"

class Platform::PlatformImpl
{
public:
  PlatformImpl(int densityDpi)
  { // Constants are taken from android.util.DisplayMetrics
    switch (densityDpi)
    {
    case 120: m_visualScale = 0.75; m_skinName = "basic_ldpi.skn"; break;
    case 160: m_visualScale = 1.0; m_skinName = "basic_mdpi.skn"; break;
    case 240: m_visualScale = 1.5; m_skinName = "basic_hdpi.skn"; break;
    default: m_visualScale = 2.0; m_skinName = "basic_xhdpi.skn"; break;
    }
  }
  double m_visualScale;
  string m_skinName;
};

double Platform::VisualScale() const
{
  return m_impl->m_visualScale;
}

string Platform::SkinName() const
{
  return m_impl->m_skinName;
}

namespace android
{
  Platform::~Platform()
  {
    delete m_impl;
  }

  void Platform::Initialize(int densityDpi, string const & apkPath,
      jint screenWidth, jint screenHeight,
      string const & storagePath, string const & tmpPath,
      string const & extTmpPath, string const & settingsPath)
  {
    m_impl = new PlatformImpl(densityDpi, screenWidth, screenHeight);

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
