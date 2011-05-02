#pragma once

#include "download_manager.hpp"

#include <QtNetwork/QtNetwork>

class QtDownloadManager;

class QtDownload : public QObject
{
private:
  Q_OBJECT

  // can be changed during server redirections
  QUrl m_currentUrl;
  QNetworkReply * m_reply;
  QFile m_file;
  int m_retryCounter;

  HttpStartParams m_params;

  void StartRequest();

private slots:
  void OnHttpFinished();
  void OnHttpReadyRead();
  void OnUpdateDataReadProgress(qint64 bytesRead, qint64 totalBytes);

public:
  /// Created instance is automanaged as a manager's child
  /// and can be manipulated later by manager->findChild(url);
  QtDownload(QtDownloadManager & manager, HttpStartParams const & params);
  virtual ~QtDownload();
};
