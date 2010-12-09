#pragma once

#include "download_manager.hpp"

#include <QtNetwork/QtNetwork>

class QtDownloadManager;

class QtDownload : public QObject
{
private:
  Q_OBJECT

  QUrl m_currentUrl;
  QNetworkReply * m_reply;
  QFile * m_file;
  bool m_httpRequestAborted;
  TDownloadFinishedFunction m_finish;
  TDownloadProgressFunction m_progress;
  int m_retryCounter;

  QtDownload(QtDownloadManager & manager, char const * url, char const * fileName,
             TDownloadFinishedFunction & finish, TDownloadProgressFunction & progress,
             bool useResume);
  void StartRequest();

private slots:
  void OnHttpFinished();
  void OnHttpReadyRead();
  void OnUpdateDataReadProgress(qint64 bytesRead, qint64 totalBytes);

public:
  /// Created instance is automanaged as a manager's child
  /// and can be manipulated later by manager->findChild(url);
  static void StartDownload(QtDownloadManager & manager, char const * url, char const * fileName,
         TDownloadFinishedFunction & finish, TDownloadProgressFunction & progress, bool useResume);
  virtual ~QtDownload();
};
