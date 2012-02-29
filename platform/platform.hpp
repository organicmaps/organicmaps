#pragma once

#include "../coding/reader.hpp"

#include "../base/exception.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/utility.hpp"
#include "../std/function.hpp"

#include "../defines.hpp"

DECLARE_EXCEPTION(FileAbsentException, RootException);
DECLARE_EXCEPTION(NotImplementedException, RootException);

class Platform
{
protected:
  /// Usually read-only directory for application resources
  string m_resourcesDir;
  /// Writable directory to store downloaded map data
  /// @note on some systems it can point to external ejectable storage
  string m_writableDir;
  /// Temporary directory, can be cleaned up by the system
  string m_tmpDir;
  /// Writable directory to store persistent application data
  string m_settingsDir;

  class PlatformImpl;
  /// Used only on those platforms where needed
  PlatformImpl * m_impl;

  static bool IsFileExistsByFullPath(string const & filePath);

  /// Internal function to use files from writable dir
  /// if they override the same file in the resources dir
  string ReadPathForFile(string const & file) const
  {
    string fullPath = m_writableDir + file;
    if (!IsFileExistsByFullPath(fullPath))
    {
      fullPath = m_resourcesDir + file;
      if (!IsFileExistsByFullPath(fullPath))
        MYTHROW(FileAbsentException, ("File doesn't exist", fullPath));
    }
    return fullPath;
  }

public:
  Platform();
  ~Platform();

  /// @return always the same writable dir for current user with slash at the end
  string WritableDir() const { return m_writableDir; }
  /// @return full path to file in user's writable directory
  string WritablePathForFile(string const & file) const { return WritableDir() + file; }

  /// @return resource dir (on some platforms it's differ from Writable dir)
  string ResourcesDir() const { return m_resourcesDir; }

  /// @return path for directory with temporary files with slash at the end
  string TmpDir() const { return m_tmpDir; }
  /// @return full path to file in the temporary directory
  string TmpPathForFile(string const & file) const { return TmpDir() + file; }

  /// @return path for directory in the persistent memory, can be the same
  /// as WritableDir, but on some platforms it's different
  string SettingsDir() const { return m_settingsDir; }
  /// @return full path to file in the settings directory
  string SettingsPathForFile(string const & file) const { return SettingsDir() + file; }

  /// @return reader for file decriptor.
  /// @throws FileAbsentException
  /// @param[in] file descriptor which we want to read
  ModelReader * GetReader(string const & file) const;

  /// @name File operations
  //@{
  typedef vector<string> FilesList;
  /// Retrieves files list contained in given directory
  /// @param directory directory path with slash at the end
  /// @param mask files extension to find, like ".map" etc
  /// @return number of files found in outFiles
  static void GetFilesInDir(string const & directory, string const & mask, FilesList & outFiles);
  /// @return false if file is not exist
  /// @note Check files in Writable dir first, and in ReadDir if not exist in Writable dir
  bool GetFileSizeByName(string const & fileName, uint64_t & size) const;
  /// @return false if file is not exist
  /// @note Try do not use in client production code
  static bool GetFileSizeByFullPath(string const & filePath, uint64_t & size);
  //@}

  /// @name Functions for concurrent tasks.
  //@{
  typedef function<void()> TFunctor;
  void RunOnGuiThread(TFunctor const & fn);
  enum Priority
  {
    EPriorityBackground,
    EPriorityLow,
    EPriorityDefault,
    EPriorityHigh
  };
  void RunAsync(TFunctor const & fn, Priority p = EPriorityDefault);
  //@}

  int CpuCores() const;

  double VisualScale() const;

  string SkinName() const;

  void GetFontNames(FilesList & res) const;

  bool IsMultiThreadedRendering() const;

  int TileSize() const;

  int MaxTilesCount() const;

  int VideoMemoryLimit() const;

  int PreCachingDepth() const;

  string DeviceName() const;

  int ScaleEtalonSize() const;

  string UniqueClientId() const;

  /// @return true for "search" feature if app needs search functionality
  bool IsFeatureSupported(string const & feature) const;

  /// @return url for clients to download maps
  /// Different urls are returned for versions with and without search support
  inline string MetaServerUrl() const
  {
    if (IsFeatureSupported("search"))
      return "http://active.servers.url";
    else
      return "http://active.servers.url";
  }

  /// @return JSON-encoded list of urls if metaserver is unreachable
  inline string DefaultUrlsJSON() const
  {
    if (IsFeatureSupported("search"))
      return "[\"http://1st.default.server/\",\"http://2nd.default.server/\",\"http://3rd.default.server/\"]";
    else
      return "[\"http://1st.default.server/\",\"http://2nd.default.server/\",\"http://3rd.default.server/\"]";
  }
};

extern "C" Platform & GetPlatform();
