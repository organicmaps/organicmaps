#include "Platform.hpp"

#include "../core/jni_string.hpp"
#include <math.h>

#include "../../../../../base/logging.hpp"

class Platform::PlatformImpl
{
public:
  PlatformImpl(int densityDpi, int screenWidth, int screenHeight)
  { // Constants are taken from android.util.DisplayMetrics

    /// ceiling screen sizes to the nearest power of two, and taking half of it as a tile size

    double const log2 = log(2.0);

    screenWidth = static_cast<int>(pow(2.0, ceil(log(double(screenWidth)) / log2)));
    screenHeight = static_cast<int>(pow(2.0, ceil(log(double(screenHeight)) / log2)));

    m_tileSize = min(max(max(screenWidth, screenHeight) / 2, 128), 512);

    int k = static_cast<int>((256.0 / m_tileSize) * (256.0 / m_tileSize));

    /// calculating how much tiles we need for the screen of such size

    /// pure magic ;)

    double rotatedScreenCircleDiameter = sqrt(screenWidth * screenWidth + screenHeight * screenHeight);
    int tilesOnOneSide = ceil(rotatedScreenCircleDiameter / (m_tileSize / 1.05 / 2));
    int singleScreenTilesCount = tilesOnOneSide * tilesOnOneSide;
    m_maxTilesCount = singleScreenTilesCount * 2;

    LOG(LINFO, ("minimum amount of tiles needed is", m_maxTilesCount));

    m_maxTilesCount = max(120 * k, m_maxTilesCount);

    switch (densityDpi)
    {
    case 120:
      m_visualScale = 0.75;
      m_skinName = "basic_ldpi.skn";
      LOG(LINFO, ("using LDPI resources"));
      break;
    case 160:
      m_visualScale = 1.0;
      m_skinName = "basic_mdpi.skn";
      LOG(LINFO, ("using MDPI resources"));
      break;
    case 240:
      m_visualScale = 1.5;
      m_skinName = "basic_hdpi.skn";
      LOG(LINFO, ("using HDPI resources"));
      break;
    default:
      m_visualScale = 2.0;
      m_skinName = "basic_xhdpi.skn";
      LOG(LINFO, ("using XHDPI resources"));
      break;
    }
  }
  double m_visualScale;
  string m_skinName;
  int m_maxTilesCount;
  size_t m_tileSize;
};

double Platform::VisualScale() const
{
  return m_impl->m_visualScale;
}

string Platform::SkinName() const
{
  return m_impl->m_skinName;
}

int Platform::MaxTilesCount() const
{
  return m_impl->m_maxTilesCount;
}

int Platform::TileSize() const
{
  return m_impl->m_tileSize;
}

namespace android
{
  Platform::~Platform()
  {
    delete m_impl;
  }

  void Platform::Initialize(JNIEnv * env,
                            jint densityDpi,
                            jint screenWidth,
                            jint screenHeight,
                            jstring apkPath,
                            jstring storagePath,
                            jstring tmpPath,
                            jstring extTmpPath,
                            jstring settingsPath)
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
