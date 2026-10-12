#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>

class Framework;

namespace sailfish
{
class BookmarksIO;
class MapsStorage;

// Opens bookmark and track files, and geo:, om://, ge0:// and https://omaps.app links. They come as arguments or
// over D-Bus from the launcher, see organicmaps.desktop.
class UrlHandler : public QObject
{
  Q_OBJECT
  // The service, object path and interface of the launcher and notifications are set in CMakeLists.txt.
  Q_CLASSINFO("D-Bus Interface", SAILFISH_DBUS_SERVICE)
  Q_PROPERTY(QString dbusService READ dbusService CONSTANT)
  Q_PROPERTY(QString dbusPath READ dbusPath CONSTANT)

public:
  UrlHandler(Framework & framework, BookmarksIO & bookmarksIO, MapsStorage const & mapsStorage,
             QObject * parent = nullptr);

  QString dbusService() const { return QStringLiteral(SAILFISH_DBUS_SERVICE); }
  QString dbusPath() const { return QStringLiteral(SAILFISH_DBUS_PATH); }

  // Takes the D-Bus name of the app, before the framework starts; false when another instance has it.
  static bool ClaimService();
  // For the launcher and the notifications, on the claimed name.
  bool RegisterOnDBus();
  static void OpenInRunningApp(QStringList const & urls);

public slots:
  // The X-Maemo-Method of organicmaps.desktop.
  Q_SCRIPTABLE void openUrl(QStringList const & urls);
  // Notification actions.
  Q_SCRIPTABLE void cancelDownloads() { emit cancelDownloadsRequested(); }
  Q_SCRIPTABLE void stopTrackRecording() { emit stopTrackRecordingRequested(); }

signals:
  void activated();
  // Points as {lat, lon, name}, from start to finish.
  void routeRequested(int routerType, QVariantList const & points);
  void searchRequested(QString const & query, bool onMap);
  // An om://crosshair link: pick a position for the app named, which may give a link back.
  void crosshairRequested(QString const & appName, QString const & backUrl);
  void oauth2CodeReceived(QString const & code);
  void cancelDownloadsRequested();
  void stopTrackRecordingRequested();

private:
  void ProcessPending();
  void Process(QString const & url);

  Framework & m_framework;
  BookmarksIO & m_bookmarksIO;
  MapsStorage const & m_mapsStorage;
  // Links wait for the map, which their requests move.
  QStringList m_pending;
  QTimer m_retryTimer;
};
}  // namespace sailfish
