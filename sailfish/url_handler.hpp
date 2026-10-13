#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

class Framework;

namespace sailfish
{
class BookmarksIO;

class UrlHandler;

// The D-Bus interface of the launcher and notifications; the service, object path and interface are set in
// CMakeLists.txt. It is served before the name is taken, so that calls while the app starts wait for UrlHandler.
class DBusInbox : public QObject
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", SAILFISH_DBUS_SERVICE)

public:
  // Before the framework starts; false when another instance has the name.
  bool Claim();
  static void OpenInRunningApp(QStringList const & urls);
  // Also gets the links received until now.
  void SetHandler(UrlHandler & handler);

public slots:
  // The X-Maemo-Method of organicmaps.desktop.
  Q_SCRIPTABLE void openUrl(QStringList const & urls);
  // Notification actions.
  Q_SCRIPTABLE void cancelDownloads();
  Q_SCRIPTABLE void stopTrackRecording();

private:
  UrlHandler * m_handler = nullptr;
  QStringList m_queued;
};

// Opens bookmark and track files, and geo:, om://, ge0:// and https://omaps.app links. They come as arguments or
// over D-Bus from the launcher, see organicmaps.desktop.
class UrlHandler : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString dbusService READ dbusService CONSTANT)
  Q_PROPERTY(QString dbusPath READ dbusPath CONSTANT)

public:
  UrlHandler(Framework & framework, BookmarksIO & bookmarksIO, QObject * parent = nullptr);

  QString dbusService() const { return QStringLiteral(SAILFISH_DBUS_SERVICE); }
  QString dbusPath() const { return QStringLiteral(SAILFISH_DBUS_PATH); }

  void openUrl(QStringList const & urls);
  // Opens the links that waited for the map.
  Q_INVOKABLE void processPending();

signals:
  void activated();
  // Points as {lat, lon, name}, from start to finish.
  void routeRequested(int routerType, QVariantList const & points);
  // The locale is empty when the link has none.
  void searchRequested(QString const & query, QString const & locale, bool onMap);
  // An om://crosshair link: pick a position for the app named, which may give a link back.
  void crosshairRequested(QString const & appName, QString const & backUrl);
  void oauth2CodeReceived(QString const & code);
  void cancelDownloadsRequested();
  void stopTrackRecordingRequested();

private:
  void Process(QString const & url);

  Framework & m_framework;
  BookmarksIO & m_bookmarksIO;
  // Links wait for the map, which their requests move.
  QStringList m_pending;
};
}  // namespace sailfish
