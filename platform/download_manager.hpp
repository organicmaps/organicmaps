#pragma once

#include "../std/stdint.hpp"
#include "../std/utility.hpp"
#include "../std/string.hpp"
#include "../std/function.hpp"

/// Appended to all downloading files and removed after successful download
#define DOWNLOADING_FILE_EXTENSION ".downloading"

/// Notifies client about donwload progress
struct HttpProgressT
{
  string m_url;
  int64_t m_current;
  int64_t m_total;
};
typedef function<void (HttpProgressT const &)> HttpProgressCallbackT;

enum DownloadResultT
{
  EHttpDownloadOk,
  EHttpDownloadFileNotFound,          //!< HTTP 404
  EHttpDownloadFailed,
  EHttpDownloadFileIsLocked,          //!< downloaded file can't replace existing locked file
  EHttpDownloadCantCreateFile,        //!< file for downloading can't be created
  EHttpDownloadNoConnectionAvailable
};

struct HttpFinishedParams
{
  string m_url;
  string m_file;    //!< if not empty, contains file with retrieved data
  string m_data;    //!< if not empty, contains received data
  DownloadResultT m_error;
};
typedef function<void (HttpFinishedParams const &)> HttpFinishedCallbackT;

struct HttpStartParams
{
  string m_url;
  string m_fileToSave;
  HttpFinishedCallbackT m_finish;
  HttpProgressCallbackT m_progress;
  string m_contentType;
  string m_postData;        //!< if not empty, send POST instead of GET
};

/// Platform-dependent implementations should derive it
/// and implement pure virtual methods
class DownloadManager
{
public:
  virtual ~DownloadManager() {}

  /// By default, http resume feature is used for requests which contains out file
  /// If url doesn't contain http:// or https:// Url_Generator is used for base server url
  virtual void HttpRequest(HttpStartParams const & params) = 0;
  /// @note Doesn't notifies clients on canceling!
  virtual void CancelDownload(string const & url) = 0;
  virtual void CancelAllDownloads() = 0;
};

extern "C" DownloadManager & GetDownloadManager();
