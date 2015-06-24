#pragma once

#include "std/string.hpp"
#include "std/cstdint.hpp"

#include <QObject>
#include <QNetworkReply>

namespace downloader { class IHttpThreadCallback; }

class HttpThread : public QObject
{
  Q_OBJECT

public:
  HttpThread(string const & url,
                         downloader::IHttpThreadCallback & cb,
                         int64_t beg,
                         int64_t end,
                         int64_t size,
                         string const & pb);
  virtual ~HttpThread();

private slots:
  void OnHeadersReceived();
  void OnChunkDownloaded();
  void OnDownloadFinished();

private:
  downloader::IHttpThreadCallback & m_callback;
  QNetworkReply * m_reply;
  int64_t m_begRange;
  int64_t m_endRange;
  int64_t m_downloadedBytes;
  int64_t m_expectedSize;
};
