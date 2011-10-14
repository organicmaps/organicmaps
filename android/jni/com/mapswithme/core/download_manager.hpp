/*
 * download_manager.hpp
 *
 *  Created on: Oct 14, 2011
 *      Author: siarheirachytski
 */
#pragma once

#include "../../../../../platform/download_manager.hpp"

namespace android
{
  class DownloadManager : public ::DownloadManager
  {
  public:

    virtual void HttpRequest(HttpStartParams const & params);
    virtual void CancelDownload(string const & url);
    virtual void CancelAllDownloads();
  };
}

DownloadManager & GetDownloadManager();
