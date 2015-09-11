#include "pagedownloader.h"
#include "logging.h"

#include <QUrl>
#include <QNetworkReply>


void PageDownloader::ConnectFinished(QObject * obj, char const * slot)
{
  disconnect(SIGNAL(finished(QString const &)));
  connect(this, SIGNAL(finished(QString const &)), obj, slot);
}

void PageDownloader::Download(QUrl const & url)
{
  m_res.clear();

  m_reply = m_manager.get(QNetworkRequest(url));
  connect(m_reply, SIGNAL(finished()), this, SLOT(httpFinished()));
  connect(m_reply, SIGNAL(readyRead()), this, SLOT(httpReadyRead()));
  connect(m_reply, SIGNAL(downloadProgress(qint64,qint64)), this,
          SLOT(updateDataReadProgress(qint64,qint64)));
}

void PageDownloader::Download(QString const & url)
{
  Download(QUrl(url));
}

void PageDownloader::httpFinished()
{
  QString const s = QString::fromUtf8(m_res.constData());
  QString const url = m_reply->url().toString();

  if (s.isEmpty())
  {
    m_log.Print(Logging::WARNING, QString("Downloading of ") +
                                  url +
                                  QString(" failed."));
  }
  else
  {
    m_log.Print(Logging::INFO, QString("Downloading of ") +
                               url +
                               QString(" finished successfully."));
  }

  m_reply->deleteLater();
  m_reply = 0;

  emit finished(s);
}

void PageDownloader::httpReadyRead()
{
  m_res += m_reply->readAll();
}

void PageDownloader::updateDataReadProgress(qint64 read, qint64 total)
{
  m_log.Percent(read, total);
}
