#include "sailfish/media_pause.hpp"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>

namespace sailfish
{
namespace
{
QString const kMprisPrefix = QStringLiteral("org.mpris.MediaPlayer2.");

QDBusMessage PlayerCall(QString const & service, QString const & method)
{
  return QDBusMessage::createMethodCall(service, QStringLiteral("/org/mpris/MediaPlayer2"),
                                        QStringLiteral("org.mpris.MediaPlayer2.Player"), method);
}
}  // namespace

void MediaPause::Pause()
{
  if (m_paused)
    return;
  m_paused = true;
  auto const pause = ++m_pause;
  auto const watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(
      QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.DBus"), QStringLiteral("/org/freedesktop/DBus"),
                                     QStringLiteral("org.freedesktop.DBus"), QStringLiteral("ListNames"))));
  connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, pause](QDBusPendingCallWatcher * call)
  {
    call->deleteLater();
    QDBusPendingReply<QStringList> const reply = *call;
    if (pause != m_pause || reply.isError())
      return;
    for (auto const & name : reply.value())
      if (name.startsWith(kMprisPrefix))
        PauseIfPlaying(name);
  });
}

void MediaPause::PauseIfPlaying(QString const & service)
{
  auto const pause = m_pause;
  auto const watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(
      QDBusMessage::createMethodCall(service, QStringLiteral("/org/mpris/MediaPlayer2"),
                                     QStringLiteral("org.freedesktop.DBus.Properties"), QStringLiteral("Get"))
      << QStringLiteral("org.mpris.MediaPlayer2.Player") << QStringLiteral("PlaybackStatus")));
  connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, pause, service](QDBusPendingCallWatcher * call)
  {
    call->deleteLater();
    QDBusPendingReply<QDBusVariant> const reply = *call;
    if (pause != m_pause || reply.isError() || reply.value().variant().toString() != QLatin1String("Playing"))
      return;
    QDBusConnection::sessionBus().asyncCall(PlayerCall(service, QStringLiteral("Pause")));
    m_players.append(service);
  });
}

void MediaPause::Resume()
{
  if (!m_paused)
    return;
  m_paused = false;
  ++m_pause;
  for (auto const & service : m_players)
    QDBusConnection::sessionBus().asyncCall(PlayerCall(service, QStringLiteral("Play")));
  m_players.clear();
}
}  // namespace sailfish
