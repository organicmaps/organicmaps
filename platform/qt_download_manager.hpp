#include "download_manager.hpp"

#include <QtNetwork/QtNetwork>

class QtDownloadManager : public QObject, public DownloadManager
{
  Q_OBJECT

  QNetworkAccessManager m_netAccessManager;

public:
  virtual void HttpRequest(HttpStartParams const & params);
  virtual void CancelDownload(string const & url);
  virtual void CancelAllDownloads();

  QNetworkAccessManager & NetAccessManager() { return m_netAccessManager; }
};
