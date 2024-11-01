#pragma once

#include <QNetworkReply>
#include <QObject>

#include "network/http/thread.hpp"

namespace om::network::http
{
class Thread : public QObject
{
  Q_OBJECT

public:
  Thread(std::string const & url, IThreadCallback & cb, int64_t beg, int64_t end, int64_t size, std::string const & pb);
  ~Thread() override;

private slots:
  void OnHeadersReceived();
  void OnChunkDownloaded();
  void OnDownloadFinished();

private:
  IThreadCallback & m_callback;
  QNetworkReply * m_reply;
  int64_t m_begRange;
  int64_t m_endRange;
  int64_t m_downloadedBytes;
  int64_t m_expectedSize;
};
}  // namespace om::network::http
