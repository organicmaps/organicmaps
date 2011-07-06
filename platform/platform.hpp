#pragma once

#include "../coding/reader.hpp"

#include "../base/exception.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/utility.hpp"


DECLARE_EXCEPTION(FileAbsentException, RootException);
DECLARE_EXCEPTION(NotImplementedException, RootException);


class Platform
{
public:
  virtual ~Platform() {}

  /// @return always the same writable dir for current user with slash at the end
  virtual string WritableDir() const = 0;
  /// @return full path to file in user's writable directory
  string WritablePathForFile(string const & file) const
  {
    return WritableDir() + file;
  }

  /// @return resource dir (on some platforms it's differ from Writable dir)
  virtual string ResourcesDir() const = 0;

  /// @return reader for file decriptor.
  /// @throws FileAbsentException
  /// @param[in] file descriptor which we want to read
  virtual ModelReader * GetReader(string const & file) const = 0;

  /// @name File operations
  //@{
  typedef vector<string> FilesList;
  /// Retrieves files list contained in given directory
  /// @param directory directory path with slash at the end
  /// @param mask files extension to find, like ".map" etc
  /// @return number of files found in outFiles
  virtual void GetFilesInDir(string const & directory, string const & mask, FilesList & outFiles) const = 0;
  /// @return false if file is not exist
  virtual bool GetFileSize(string const & file, uint64_t & size) const = 0;
  /// Renamed to avoid conflict with Windows macroses
  virtual bool RenameFileX(string const & original, string const & newName) const = 0;
  /// Simple file existing check
  bool IsFileExists(string const & file) const
  {
    uint64_t dummy;
    return GetFileSize(file, dummy);
  }
  //@}

  virtual int CpuCores() const = 0;

  virtual double VisualScale() const = 0;

  virtual string SkinName() const = 0;

  virtual void GetFontNames(FilesList & res) const = 0;

  virtual bool IsBenchmarking() const = 0;

  virtual int TileSize() const = 0;

  virtual int MaxTilesCount() const = 0;

  virtual bool IsVisualLog() const = 0;

  virtual string DeviceID() const = 0;

  virtual int ScaleEtalonSize() const = 0;
};

class BasePlatformImpl : public Platform
{
protected:
  string m_writableDir, m_resourcesDir;

public:
  virtual string WritableDir() const { return m_writableDir; }
  virtual string ResourcesDir() const { return m_resourcesDir; }
  virtual ModelReader * GetReader(string const & file) const;

  virtual void GetFilesInDir(string const & directory, string const & mask, FilesList & res) const;
  virtual bool GetFileSize(string const & file, uint64_t & size) const;
  virtual bool RenameFileX(string const & fOld, string const & fNew) const;
  virtual void GetFontNames(FilesList & res) const;

  virtual double VisualScale() const;
  virtual string SkinName() const;
  virtual bool IsBenchmarking() const;
  virtual bool IsVisualLog() const;
  virtual int ScaleEtalonSize() const;
  virtual int TileSize() const;
  virtual int MaxTilesCount() const;
};

extern "C" Platform & GetPlatform();
