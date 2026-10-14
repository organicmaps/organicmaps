#include "sailfish/url_handler.hpp"

#include "sailfish/bookmarks_io.hpp"
#include "sailfish/maps_storage.hpp"

#include "map/framework.hpp"
#include "map/mwm_url.hpp"

#include "geometry/mercator.hpp"

#include "base/logging.hpp"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QVariantMap>

namespace sailfish
{
namespace
{
char constexpr kService[] = SAILFISH_DBUS_SERVICE;
char constexpr kPath[] = SAILFISH_DBUS_PATH;
// SEARCH_IN_VIEWPORT_ZOOM on Android.
int constexpr kLinkZoom = 16;
}  // namespace

UrlHandler::UrlHandler(Framework & framework, BookmarksIO & bookmarksIO, MapsStorage const & mapsStorage,
                       QObject * parent)
  : QObject(parent)
  , m_framework(framework)
  , m_bookmarksIO(bookmarksIO)
  , m_mapsStorage(mapsStorage)
{
  m_retryTimer.setInterval(200);
  connect(&m_retryTimer, &QTimer::timeout, this, &UrlHandler::ProcessPending);
}

// static
bool UrlHandler::ClaimService()
{
  auto bus = QDBusConnection::sessionBus();
  if (bus.registerService(kService))
    return true;
  LOG(LINFO, ("D-Bus service taken by another instance", bus.lastError().message().toStdString()));
  return false;
}

bool UrlHandler::RegisterOnDBus()
{
  return QDBusConnection::sessionBus().registerObject(kPath, this, QDBusConnection::ExportScriptableSlots);
}

// static
void UrlHandler::OpenInRunningApp(QStringList const & urls)
{
  auto message = QDBusMessage::createMethodCall(kService, kPath, kService, QStringLiteral("openUrl"));
  message << urls;
  QDBusConnection::sessionBus().call(message);
}

void UrlHandler::openUrl(QStringList const & urls)
{
  emit activated();
  if (m_mapsStorage.Locked())
  {
    LOG(LWARNING, ("Ignoring links while the maps move", urls.join(' ').toStdString()));
    return;
  }
  for (auto const & url : urls)
  {
    // Files don't need the map.
    if (!m_bookmarksIO.OpenFile(url))
      m_pending.append(url);
  }
  ProcessPending();
}

void UrlHandler::ProcessPending()
{
  if (m_pending.isEmpty())
  {
    m_retryTimer.stop();
    return;
  }
  if (!m_framework.IsDrapeEngineCreated())
  {
    m_retryTimer.start();
    return;
  }
  m_retryTimer.stop();
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
  case UrlType::Route:
  {
    auto const data = m_framework.GetParsedRoutingData();
    QVariantList points;
    for (auto const & point : data.m_points)
    {
      auto const ll = mercator::ToLatLon(point.m_org);
      points.append(QVariantMap{{"lat", ll.m_lat}, {"lon", ll.m_lon}, {"name", QString::fromStdString(point.m_name)}});
    }
    emit routeRequested(static_cast<int>(data.m_type), points);
    break;
  }
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
    if (!request.m_query.empty() && request.m_query.front() == '?')
    {
      LOG(LWARNING, ("Ignored a debug command in a link"));
      break;
    }
    if (!request.m_query.empty())
      emit searchRequested(QString::fromStdString(request.m_query), request.m_isSearchOnMap);
    break;
  }
  case UrlType::OAuth2: emit oauth2CodeReceived(QString::fromStdString(m_framework.GetParsedOAuth2Code())); break;
  default: LOG(LWARNING, ("Unsupported link", url.toStdString())); break;
  }
}
}  // namespace sailfish
