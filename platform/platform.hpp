#pragma once

#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../std/utility.hpp"

#include "../base/start_mem_debug.hpp"

class Platform
{
public:
  virtual ~Platform() {}

  /// Time in seconds passed from application start
  virtual double TimeInSec() = 0;

  /// Full path to read/write project data directory with slash at the end
  virtual string WorkingDir() = 0;
  /// Full path to read only program resources dir with slash at the end
  virtual string ResourcesDir() = 0;

  /// @name File operations
  //@{
  typedef vector<string> FilesList;
  /// Retrieves files list contained in given directory
  /// @param directory directory path with slash at the end
  /// @param mask files extension to find, like ".map" etc
  /// @return number of files found in outFiles
  virtual int GetFilesInDir(string const & directory, string const & mask, FilesList & outFiles) = 0;
  /// @return false if file is not exist
  virtual bool GetFileSize(string const & file, uint64_t & size) = 0;
  /// Renamed to avoid conflict with Windows macroses
  virtual bool RenameFileX(string const & original, string const & newName) = 0;
  /// Simple check
  bool IsFileExists(string const & file)
  {
    uint64_t dummy;
    return GetFileSize(file, dummy);
  }
  //@}

  virtual int CpuCores() = 0;

  virtual double VisualScale() const = 0;

  virtual string const SkinName() const = 0;

  virtual bool IsMultiSampled() const = 0;
};

extern "C" Platform & GetPlatform();

#include "../base/stop_mem_debug.hpp"
