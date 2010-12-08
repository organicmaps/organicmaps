#include "download_manager.hpp"

#include <QtNetwork/QtNetwork>

class QtDownloadManager : public QObject, public DownloadManager
{
  Q_OBJECT

  QNetworkAccessManager m_netAccessManager;

public:
  virtual void DownloadFile(char const * url, char const * fileName,
          TDownloadFinishedFunction finish, TDownloadProgressFunction progress,
          bool useResume);
  virtual void CancelDownload(char const * url);
  virtual void CancelAllDownloads();

  QNetworkAccessManager & NetAccessManager() { return m_netAccessManager; }
};
