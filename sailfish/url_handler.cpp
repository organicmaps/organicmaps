#include "sailfish/url_handler.hpp"

#include "map/framework.hpp"
#include "map/mwm_url.hpp"

#include "base/logging.hpp"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusReply>

#include <utility>

namespace sailfish
{
namespace
{
char constexpr kService[] = SAILFISH_DBUS_SERVICE;
char constexpr kPath[] = SAILFISH_DBUS_PATH;
}  // namespace

UrlHandler::UrlHandler(Framework & framework, QObject * parent) : QObject(parent), m_framework(framework) {}

DBusInbox::ClaimResult DBusInbox::Claim()
{
  auto bus = QDBusConnection::sessionBus();
  if (!bus.isConnected())
  {
    LOG(LWARNING, ("No D-Bus session bus:", bus.lastError().message().toStdString()));
    return ClaimResult::Failed;
  }
  if (!bus.registerObject(kPath, this, QDBusConnection::ExportScriptableSlots))
    LOG(LWARNING, ("Can't serve the D-Bus interface: links from other apps won't open"));
  QDBusReply<QDBusConnectionInterface::RegisterServiceReply> const reply = bus.interface()->registerService(kService);
  if (reply.isValid() && reply.value() == QDBusConnectionInterface::ServiceRegistered)
    return ClaimResult::Owner;
  bus.unregisterObject(kPath);
  if (reply.isValid())
  {
    LOG(LINFO, ("D-Bus service taken by another instance"));
    return ClaimResult::OtherInstance;
  }
  LOG(LWARNING, ("Can't register the D-Bus service:", reply.error().message().toStdString()));
  return ClaimResult::Failed;
}

// static
bool DBusInbox::OpenInRunningApp(QStringList const & urls)
{
  auto message = QDBusMessage::createMethodCall(kService, kPath, kService, QStringLiteral("openUrl"));
  message << urls;
  QDBusMessage const reply = QDBusConnection::sessionBus().call(message);
  if (reply.type() == QDBusMessage::ReplyMessage)
    return true;
  LOG(LWARNING, ("Can't pass the links to the running instance:", reply.errorMessage().toStdString()));
  return false;
}

void DBusInbox::SetHandler(UrlHandler & handler)
{
  m_handler = &handler;
  if (!m_queued.isEmpty())
    handler.openUrl(std::exchange(m_queued, {}));
}

void DBusInbox::openUrl(QStringList const & urls)
{
  if (m_handler)
    m_handler->openUrl(urls);
  else
    m_queued.append(urls);
}

void UrlHandler::openUrl(QStringList const & urls)
{
  emit activated();
  m_pending.append(urls);
  processPending();
}

void UrlHandler::processPending()
{
  if (m_pending.isEmpty() || !m_framework.IsDrapeEngineCreated())
    return;
  auto const pending = std::move(m_pending);
  m_pending.clear();
  for (auto const & url : pending)
    Process(url);
}

void UrlHandler::Process(QString const & url)
{
  using UrlType = url_scheme::ParsedMapApi::UrlType;
  auto const type = m_framework.ParseAndSetApiURL(url.toStdString());
  switch (type)
  {
  case UrlType::Map: m_framework.ExecuteMapApiRequest(); break;
  default: LOG(LWARNING, ("Unsupported link type", type)); break;
  }
}
}  // namespace sailfish
