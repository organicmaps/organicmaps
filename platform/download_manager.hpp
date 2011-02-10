#pragma once

#include "../std/stdint.hpp"
#include "../std/utility.hpp"

#include <boost/function.hpp>

/// Appended to all downloading files and removed after successful download
#define DOWNLOADING_FILE_EXTENSION ".downloading"

typedef std::pair<int64_t, int64_t> TDownloadProgress;
typedef boost::function<void (char const *, TDownloadProgress)> TDownloadProgressFunction;
enum DownloadResult
{
  EHttpDownloadOk,
  EHttpDownloadFileNotFound,  // HTTP 404
  EHttpDownloadFailed,
  EHttpDownloadFileIsLocked,  // downloaded file can't replace existing locked file
  EHttpDownloadCantCreateFile, // file for downloading can't be created
  EHttpDownloadNoConnectionAvailable
};
typedef boost::function<void (char const *, DownloadResult)> TDownloadFinishedFunction;

/// Platform-dependent implementations should derive it
/// and implement pure virtual methods
class DownloadManager
{
public:
  virtual ~DownloadManager() {}
  virtual void DownloadFile(char const * url, char const * fileName,
          TDownloadFinishedFunction finish, TDownloadProgressFunction progress,
          bool useResume = false) = 0;
  /// @note Doesn't notifies clients on canceling!
  virtual void CancelDownload(char const * url) = 0;
  virtual void CancelAllDownloads() = 0;
};

extern "C" DownloadManager & GetDownloadManager();
