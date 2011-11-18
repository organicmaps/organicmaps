#pragma once

#include <QObject>
#include <QNetworkAccessManager>


class Logging;
class QUrl;
class QNetworkReply;

class PageDownloader : public QObject
{
  Q_OBJECT

  QNetworkAccessManager m_manager;
  QNetworkReply * m_reply;

  Logging & m_log;

  QByteArray m_res;

public:
  PageDownloader(Logging & log) : m_log(log) {}

  void ConnectFinished(QObject * obj, char const * slot);

  void Download(QUrl const & url);
  void Download(QString const & url);

signals:
  void finished(QString const &);

private slots:
   void httpFinished();
   void httpReadyRead();
   void updateDataReadProgress(qint64 read, qint64 total);
};
