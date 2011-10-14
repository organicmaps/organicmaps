/*
 * download_manager.cpp
 *
 *  Created on: Oct 14, 2011
 *      Author: siarheirachytski
 */

#include "download_manager.hpp"

namespace android
{
  void DownloadManager::HttpRequest(HttpStartParams const & params)
  {}

  void DownloadManager::CancelDownload(string const & url)
  {}

  void DownloadManager::CancelAllDownloads()
  {}

}

DownloadManager & GetDownloadManager()
{
  static android::DownloadManager manager;
  return manager;
}

