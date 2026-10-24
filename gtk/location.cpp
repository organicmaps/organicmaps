#include "location.hpp"

#include "app.hpp"

#include "platform/location_service/position_filter.hpp"

#include "base/logging.hpp"

#include <geoclue.h>

#include <cmath>
#include <memory>

namespace gtk
{
struct LocationService::Request
{
  LocationService * m_self;
  GCancellable * m_cancellable;
};

gpointer LocationService::NewRequest(GCancellable * cancellable)
{
  return new Request{this, G_CANCELLABLE(g_object_ref(cancellable ? cancellable : m_cancellable))};
}

LocationService * LocationService::TakeRequest(gpointer data)
{
  std::unique_ptr<Request> const request(static_cast<Request *>(data));
  bool const cancelled = g_cancellable_is_cancelled(request->m_cancellable);
  g_object_unref(request->m_cancellable);
  return cancelled ? nullptr : request->m_self;
}

LocationService::LocationService() = default;

LocationService::~LocationService()
{
  Stop();
}

void LocationService::Start()
{
  if (m_started)
    return;
  m_started = true;
  m_cancellable = g_cancellable_new();
  // Match GClueSimple's portal selection; the native client must be observed before its first fix.
  char const * portal = g_getenv("GTK_USE_PORTAL");
  if (g_file_test("/.flatpak-info", G_FILE_TEST_EXISTS) || (portal && portal[0] == '1'))
    gclue_simple_new(kAppId, GCLUE_ACCURACY_LEVEL_EXACT, m_cancellable, &LocationService::OnSimpleReady, NewRequest());
  else
    gclue_client_proxy_create_full(kAppId, GCLUE_ACCURACY_LEVEL_EXACT, GCLUE_CLIENT_PROXY_CREATE_AUTO_DELETE,
                                   m_cancellable, &LocationService::OnClientReady, NewRequest());
  // Asynchronously: a slow or hanging sensor service must not freeze the UI.
  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, nullptr, "net.hadess.SensorProxy",
                           "/net/hadess/SensorProxy/Compass", "net.hadess.SensorProxy.Compass", m_cancellable,
                           &LocationService::OnCompassProxyReady, NewRequest());
}

void LocationService::Stop()
{
  if (!m_started)
    return;
  m_started = false;
  if (m_cancellable)
  {
    g_cancellable_cancel(m_cancellable);
    g_clear_object(&m_cancellable);
  }
  if (m_locationRequest)
  {
    g_cancellable_cancel(m_locationRequest);
    g_clear_object(&m_locationRequest);
  }
  m_clientStarted = false;
  if (m_client)
  {
    g_signal_handlers_disconnect_by_data(m_client, this);
    gclue_client_call_stop(m_client, nullptr, nullptr, nullptr);
    g_clear_object(&m_client);
  }
  if (m_simple)
  {
    g_signal_handlers_disconnect_by_data(m_simple, this);
    g_clear_object(&m_simple);
  }
  StopCompass();
}

void LocationService::LocationError(GError * error)
{
  LOG(LWARNING, ("GeoClue is not available:", error->message));
  bool const denied = g_error_matches(error, G_DBUS_ERROR, G_DBUS_ERROR_ACCESS_DENIED);
  g_error_free(error);
  Stop();
  if (m_onError)
    m_onError(denied ? location::EDenied : location::ENotSupported);
}

void LocationService::OnClientReady(GObject *, GAsyncResult * res, gpointer data)
{
  GError * error = nullptr;
  auto * client = gclue_client_proxy_create_full_finish(res, &error);
  auto * self = TakeRequest(data);
  if (!self)
  {
    g_clear_object(&client);
    g_clear_error(&error);
    return;
  }
  if (!client)
  {
    self->LocationError(error);
    return;
  }
  self->m_client = client;
  g_signal_connect(client, "notify::active", G_CALLBACK(&LocationService::OnClientActiveNotify), self);
  g_signal_connect(client, "location-updated", G_CALLBACK(&LocationService::OnLocationUpdated), self);
  gclue_client_call_start(client, self->m_cancellable, &LocationService::OnClientStarted, self->NewRequest());
}

void LocationService::OnClientStarted(GObject * source, GAsyncResult * res, gpointer data)
{
  GError * error = nullptr;
  bool const started = gclue_client_call_start_finish(GCLUE_CLIENT(source), res, &error);
  auto * self = TakeRequest(data);
  if (!self)
  {
    g_clear_error(&error);
    return;
  }
  if (!started)
  {
    self->LocationError(error);
    return;
  }
  self->m_clientStarted = true;
  if (self->CheckClientActive())
    self->LoadLocation(gclue_client_get_location(self->m_client));
}

void LocationService::OnLocationUpdated(GClueClient *, char const *, char const * newPath, gpointer data)
{
  auto * self = static_cast<LocationService *>(data);
  if (self->m_clientStarted)
    self->LoadLocation(newPath);
}

void LocationService::LoadLocation(char const * path)
{
  if (!path || g_str_equal(path, "/"))
    return;
  // A newer LocationUpdated supersedes a proxy request still in flight.
  if (m_locationRequest)
  {
    g_cancellable_cancel(m_locationRequest);
    g_clear_object(&m_locationRequest);
  }
  m_locationRequest = g_cancellable_new();
  gclue_location_proxy_new_for_bus(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, "org.freedesktop.GeoClue2", path,
                                   m_locationRequest, &LocationService::OnLocationReady, NewRequest(m_locationRequest));
}

void LocationService::OnLocationReady(GObject *, GAsyncResult * res, gpointer data)
{
  GError * error = nullptr;
  auto * loc = gclue_location_proxy_new_for_bus_finish(res, &error);
  auto * self = TakeRequest(data);
  if (self && !loc)
    self->LocationError(error);
  else
    g_clear_error(&error);
  if (self && loc)
    self->ReadLocation(loc);
  g_clear_object(&loc);
}

void LocationService::OnSimpleReady(GObject *, GAsyncResult * res, gpointer data)
{
  GError * error = nullptr;
  GClueSimple * simple = gclue_simple_new_finish(res, &error);
  auto * self = TakeRequest(data);
  if (!self)
  {
    g_clear_object(&simple);
    g_clear_error(&error);
    return;
  }
  if (!simple)
  {
    self->LocationError(error);
    return;
  }
  self->m_simple = simple;
  g_signal_connect(simple, "notify::location", G_CALLBACK(&LocationService::OnLocationNotify), self);
  self->ReadLocation(gclue_simple_get_location(simple));
}

void LocationService::OnClientActiveNotify(GObject *, GParamSpec *, gpointer data)
{
  static_cast<LocationService *>(data)->CheckClientActive();
}

bool LocationService::CheckClientActive()
{
  // Active=false is normal until Client.Start has completed.
  if (!m_clientStarted || gclue_client_get_active(m_client))
    return true;
  Stop();
  if (m_onError)
    m_onError(location::EGPSIsOff);
  return false;
}

void LocationService::OnLocationNotify(GObject *, GParamSpec *, gpointer data)
{
  auto * self = static_cast<LocationService *>(data);
  self->ReadLocation(gclue_simple_get_location(self->m_simple));
}

void LocationService::ReadLocation(GClueLocation * loc)
{
  if (!loc)
    return;

  location::GpsInfo info;
  info.m_source = location::EGeoClue2;
  info.m_latitude = gclue_location_get_latitude(loc);
  info.m_longitude = gclue_location_get_longitude(loc);
  info.m_horizontalAccuracy = gclue_location_get_accuracy(loc);
  double const altitude = gclue_location_get_altitude(loc);
  if (altitude != -G_MAXDOUBLE)
  {
    info.m_altitude = altitude;
    info.m_verticalAccuracy = info.m_horizontalAccuracy;
  }
  double const speed = gclue_location_get_speed(loc);
  if (speed >= 0)
    info.m_speed = speed;
  double const heading = gclue_location_get_heading(loc);
  // GeoClue reports the compass heading when not moving; only trust it as a bearing while moving.
  if (heading >= 0 && speed > 0.5)
    info.m_bearing = heading;

  info.m_timestamp = static_cast<double>(g_get_real_time()) / 1e6;
  if (GVariant * ts = gclue_location_get_timestamp(loc))
  {
    guint64 sec = 0, usec = 0;
    g_variant_get(ts, "(tt)", &sec, &usec);
    if (sec > 0)
      info.m_timestamp = static_cast<double>(sec) + static_cast<double>(usec) / 1e6;
  }

  if (info.m_horizontalAccuracy <= 0)
    info.m_horizontalAccuracy = 100;

  if (!location::IsCurrentLocation(info, m_last, static_cast<double>(g_get_real_time()) / 1e6))
  {
    LOG(LINFO, ("Skipping an outdated location, timestamp", info.m_timestamp));
    return;
  }
  m_last = info;
  if (m_onLocation)
    m_onLocation(info);
}

void LocationService::OnCompassProxyReady(GObject *, GAsyncResult * res, gpointer data)
{
  GError * error = nullptr;
  GDBusProxy * proxy = g_dbus_proxy_new_for_bus_finish(res, &error);
  auto * self = TakeRequest(data);
  if (!self || !proxy)
  {
    if (self)
      LOG(LINFO, ("No compass:", error->message));
    g_clear_object(&proxy);
    g_clear_error(&error);
    return;
  }
  self->m_compass = proxy;
  g_signal_connect(proxy, "notify::g-name-owner", G_CALLBACK(&LocationService::OnCompassOwnerNotify), self);
  g_signal_connect(proxy, "g-properties-changed", G_CALLBACK(&LocationService::OnCompassProperties), self);
  self->ClaimCompass();
}

void LocationService::OnCompassOwnerNotify(GObject *, GParamSpec *, gpointer data)
{
  auto * self = static_cast<LocationService *>(data);
  // A restarted service has lost our claim. Cancel replies from its previous owner.
  if (self->m_compassRequest)
  {
    g_cancellable_cancel(self->m_compassRequest);
    g_clear_object(&self->m_compassRequest);
  }
  self->m_compassClaimed = false;
  self->ClaimCompass();
}

void LocationService::ClaimCompass()
{
  char * owner = g_dbus_proxy_get_name_owner(m_compass);
  bool const available = owner != nullptr;
  g_free(owner);
  if (!available || m_compassRequest)
    return;
  // Claim even without a sensor: sensor-proxy will deliver updates when one appears.
  m_compassRequest = g_cancellable_new();
  g_dbus_proxy_call(m_compass, "ClaimCompass", nullptr, G_DBUS_CALL_FLAGS_NONE, -1, m_compassRequest,
                    &LocationService::OnCompassClaimed, NewRequest(m_compassRequest));
}

void LocationService::OnCompassClaimed(GObject * source, GAsyncResult * res, gpointer data)
{
  GError * error = nullptr;
  GVariant * result = g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, &error);
  auto * self = TakeRequest(data);
  if (result)
    g_variant_unref(result);
  if (!self)
  {
    g_clear_error(&error);
    return;
  }
  if (!result)
  {
    LOG(LINFO, ("ClaimCompass failed:", error->message));
    g_clear_error(&error);
    self->StopCompass();
    return;
  }
  self->m_compassClaimed = true;
  self->ReadCompassHeading();
}

void LocationService::StopCompass()
{
  if (!m_compass)
    return;
  if (m_compassRequest)
  {
    g_cancellable_cancel(m_compassRequest);
    g_clear_object(&m_compassRequest);
  }
  m_compassClaimed = false;
  g_signal_handlers_disconnect_by_data(m_compass, this);
  g_dbus_proxy_call(m_compass, "ReleaseCompass", nullptr, G_DBUS_CALL_FLAGS_NONE, 1000, nullptr, nullptr, nullptr);
  g_clear_object(&m_compass);
}

void LocationService::OnCompassProperties(GDBusProxy *, GVariant * changed, GStrv, gpointer data)
{
  double heading = -1;
  auto * self = static_cast<LocationService *>(data);
  if (self->m_compassClaimed && g_variant_lookup(changed, "CompassHeading", "d", &heading))
    self->OnCompassHeading(heading);
}

void LocationService::ReadCompassHeading()
{
  GVariant * value = g_dbus_proxy_get_cached_property(m_compass, "CompassHeading");
  if (!value)
    return;
  double const heading = g_variant_get_double(value);
  g_variant_unref(value);
  OnCompassHeading(heading);
}

void LocationService::OnCompassHeading(double heading)
{
  if (!std::isfinite(heading) || heading < 0)
    return;
  // The heading is magnetic while the core expects a true one: the magnetic declination of the
  // place is not corrected yet.
  if (m_onCompass)
    m_onCompass(heading * M_PI / 180.0);
}
}  // namespace gtk
