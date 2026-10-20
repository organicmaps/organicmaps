#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class Framework;

namespace sailfish
{
class UrlHandler;

// The D-Bus interface of the launcher; the service, object path and interface are set in CMakeLists.txt. It is
// served before the name is taken, so that calls while the app starts wait for UrlHandler.
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

private:
  UrlHandler * m_handler = nullptr;
  QStringList m_queued;
};

// Opens geo:, om://, ge0:// and https://omaps.app links. They come as arguments or over D-Bus from the launcher, see
// organicmaps.desktop.
class UrlHandler : public QObject
{
  Q_OBJECT

public:
  explicit UrlHandler(Framework & framework, QObject * parent = nullptr);

  void openUrl(QStringList const & urls);
  // Opens the links that waited for the map.
  Q_INVOKABLE void processPending();

signals:
  void activated();

private:
  void Process(QString const & url);

  Framework & m_framework;
  // Links wait for the map, which their requests move.
  QStringList m_pending;
};
}  // namespace sailfish
