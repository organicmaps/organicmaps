#pragma once

#include "../coding/reader.hpp"

#include "../base/exception.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/utility.hpp"
#include "../std/function.hpp"


DECLARE_EXCEPTION(FileAbsentException, RootException);
DECLARE_EXCEPTION(NotImplementedException, RootException);

class Platform
{
protected:
  string m_writableDir, m_resourcesDir;
  class PlatformImpl;
  /// Used only on those platforms where needed
  PlatformImpl * m_impl;

  static bool IsFileExistsByFullPath(string const & filePath);

  /// Internal function to use files from writable dir if they override the same in the resources
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
  inline void RunInGuiThread(TFunctor const & fn) { fn(); }
  inline void RunAsync(TFunctor const & fn) { fn(); }
  //@}

  int CpuCores() const;

  double VisualScale() const;

  string SkinName() const;

  void GetFontNames(FilesList & res) const;

  bool IsMultiThreadedRendering() const;

  int TileSize() const;

  int MaxTilesCount() const;

  int VideoMemoryLimit() const;

  string DeviceName() const;

  int ScaleEtalonSize() const;

  string UniqueClientId() const;
};

extern "C" Platform & GetPlatform();
