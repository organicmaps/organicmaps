#include "sailfish/url_handler.hpp"

#include "sailfish/bookmarks_io.hpp"

#include "map/framework.hpp"
#include "map/mwm_url.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"

#include <QDBusConnection>
#include <QDBusMessage>

#include <utility>

namespace sailfish
{
namespace
{
char constexpr kService[] = SAILFISH_DBUS_SERVICE;
char constexpr kPath[] = SAILFISH_DBUS_PATH;
// SEARCH_IN_VIEWPORT_ZOOM on Android.
int constexpr kLinkZoom = 16;
}  // namespace

UrlHandler::UrlHandler(Framework & framework, BookmarksIO & bookmarksIO, QObject * parent)
  : QObject(parent)
  , m_framework(framework)
  , m_bookmarksIO(bookmarksIO)
{}

bool DBusInbox::Claim()
{
  auto bus = QDBusConnection::sessionBus();
  if (!bus.registerObject(kPath, this, QDBusConnection::ExportScriptableSlots))
    LOG(LWARNING, ("Can't serve the D-Bus interface: links from other apps won't open"));
  if (bus.registerService(kService))
    return true;
  LOG(LINFO, ("D-Bus service taken by another instance", bus.lastError().message().toStdString()));
  bus.unregisterObject(kPath);
  return false;
}

// static
void DBusInbox::OpenInRunningApp(QStringList const & urls)
{
  auto message = QDBusMessage::createMethodCall(kService, kPath, kService, QStringLiteral("openUrl"));
  message << urls;
  QDBusConnection::sessionBus().call(message);
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

void DBusInbox::cancelDownloads()
{
  if (m_handler)
    emit m_handler->cancelDownloadsRequested();
}

void DBusInbox::stopTrackRecording()
{
  if (m_handler)
    emit m_handler->stopTrackRecordingRequested();
}

void UrlHandler::openUrl(QStringList const & urls)
{
  emit activated();
  for (auto const & url : urls)
  {
    // Files don't need the map.
    if (!m_bookmarksIO.OpenFile(url))
      m_pending.append(url);
  }
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
  case UrlType::Search:
  case UrlType::Crosshair:
  {
    if (auto const center = m_framework.GetParsedCenterLatLon(); center.IsValid())
    {
      m_framework.StopLocationFollow();
      m_framework.SetViewportCenter(mercator::FromLatLon(center), kLinkZoom);
    }
    if (type == UrlType::Crosshair)
    {
      emit crosshairRequested(QString::fromStdString(m_framework.GetParsedAppName()),
                              QString::fromStdString(m_framework.GetParsedBackUrl()));
      break;
    }
    auto const request = m_framework.GetParsedSearchRequest();
    // Search runs "?" debug commands, e.g. changing the map server: not for links, which any app can send.
    // Trimmed too, in case a command is ever matched after trimming.
    if (QString::fromStdString(request.m_query).trimmed().startsWith(QLatin1Char('?')))
    {
      LOG(LWARNING, ("Ignored a debug command in a link"));
      break;
    }
    if (!request.m_query.empty())
      emit searchRequested(QString::fromStdString(request.m_query), QString::fromStdString(request.m_locale),
                           request.m_isSearchOnMap);
    break;
  }
  default: LOG(LWARNING, ("Unsupported link", url.toStdString())); break;
  }
}
}  // namespace sailfish
