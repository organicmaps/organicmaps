#pragma once

#include "download_manager.hpp"
#include "url_generator.hpp"

#include <QtNetwork/QtNetwork>

class QtDownloadManager;

class QtDownload : public QObject
{
  Q_OBJECT

  // can be changed during server redirections
  QUrl m_currentUrl;
  QNetworkReply * m_reply;
  QFile m_file;

  HttpStartParams m_params;

  UrlGenerator m_urlGenerator;

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
