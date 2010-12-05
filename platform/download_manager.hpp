#pragma once

#include "../std/stdint.hpp"
#include "../std/utility.hpp"

#include <boost/function.hpp>

/// Appended to all downloading files and removed after successful download
#define DOWNLOADING_FILE_EXTENSION ".downloading"

typedef std::pair<uint64_t, uint64_t> TDownloadProgress;
typedef boost::function<void (char const *, TDownloadProgress)> TDownloadProgressFunction;
typedef boost::function<void (char const *, bool)> TDownloadFinishedFunction;

/// Platform-dependent implementations should derive it
/// and implement pure virtual methods
class DownloadManager
{
public:
  virtual ~DownloadManager() {}
  virtual void DownloadFile(char const * url, char const * fileName,
          TDownloadFinishedFunction finish, TDownloadProgressFunction progress) = 0;
  /// @note Doesn't notifies clients on canceling!
  virtual void CancelDownload(char const * url) = 0;
  virtual void CancelAllDownloads() = 0;
};

extern "C" DownloadManager & GetDownloadManager();
