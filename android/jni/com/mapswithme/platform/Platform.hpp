#pragma once

#include "../../../../../platform/platform.hpp"

namespace android
{
  class Platform : public ::Platform
  {
    /// External storage path for temporary files, used when external storage is available
    string m_externalTmpPath;
    /// The same but in device's internal memory (it's usually much smaller)
    string m_localTmpPath;

  public:
    ~Platform();
    void Initialize(int densityDpi, jint screenWidth, jint screenHeight,
        string const & apkPath,
        string const & storagePath, string const & tmpPath,
        string const & extTmpPath, string const & settingsPath);

    void OnExternalStorageStatusChanged(bool isAvailable);

    static Platform & Instance();
  };
}
