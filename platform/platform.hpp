#pragma once

#include "../base/exception.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/utility.hpp"

#include "../base/start_mem_debug.hpp"

DECLARE_EXCEPTION(FileAbsentException, RootException);

class Platform
{
public:
  virtual ~Platform() {}

  /// Time in seconds passed from application start
  virtual double TimeInSec() const = 0;

  /// @return always the same writable dir for current user with slash at the end
  virtual string WritableDir() const = 0;
  /// @return full path to file in user's writable directory
  string WritablePathForFile(string const & file) const
  {
    return WritableDir() + file;
  }

  /// Throws FileAbsentException
  /// @param[in] file just file name which we want to read
  /// @param[out] fullPath fully resolved path including file name
  /// @return false if file is absent
  virtual string ReadPathForFile(char const * file) const = 0;
  /// Throws FileAbsentException
  string ReadPathForFile(string const & file) const
  {
    return ReadPathForFile(file.c_str());
  }

  /// @name File operations
  //@{
  typedef vector<string> FilesList;
  /// Retrieves files list contained in given directory
  /// @param directory directory path with slash at the end
  /// @param mask files extension to find, like ".map" etc
  /// @return number of files found in outFiles
  virtual int GetFilesInDir(string const & directory, string const & mask, FilesList & outFiles) const = 0;
  /// @return false if file is not exist
  virtual bool GetFileSize(string const & file, uint64_t & size) const = 0;
  /// Renamed to avoid conflict with Windows macroses
  virtual bool RenameFileX(string const & original, string const & newName) const = 0;
  /// Simple check
  bool IsFileExists(string const & file) const
  {
    uint64_t dummy;
    return GetFileSize(file, dummy);
  }
  //@}

  virtual int CpuCores() const = 0;

  virtual double VisualScale() const = 0;

  virtual string const SkinName() const = 0;

  virtual bool IsMultiSampled() const = 0;

  virtual bool DoPeriodicalUpdate() const = 0;

  virtual vector<string> GetFontNames() const = 0;
};

extern "C" Platform & GetPlatform();

#include "../base/stop_mem_debug.hpp"
