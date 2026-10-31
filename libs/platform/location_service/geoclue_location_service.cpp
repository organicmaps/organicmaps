#include "platform/location_service/geoclue_location_service.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <geoclue.h>

#include <memory>
#include <utility>

namespace location
{
struct GeoClueLocationService::Request
{
  GeoClueLocationService * m_self;
  GCancellable * m_cancellable;
};

gpointer GeoClueLocationService::NewRequest(GCancellable * cancellable)
{
  return new Request{this, G_CANCELLABLE(g_object_ref(cancellable ? cancellable : m_cancellable))};
}

GeoClueLocationService * GeoClueLocationService::TakeRequest(gpointer data)
{
  std::unique_ptr<Request> const request(static_cast<Request *>(data));
  bool const cancelled = g_cancellable_is_cancelled(request->m_cancellable);
  g_object_unref(request->m_cancellable);
  return cancelled ? nullptr : request->m_self;
}

GeoClueLocationService::GeoClueLocationService(LocationObserver & observer, std::string desktopId,
                                               std::chrono::milliseconds firstFixTimeout)
  : LocationService(observer)
  , m_desktopId(std::move(desktopId))
  , m_firstFixTimeout(firstFixTimeout)
{}

GeoClueLocationService::~GeoClueLocationService()
{
  Stop();
}

void GeoClueLocationService::Start()
{
  if (m_started)
    return;
  m_started = true;
  m_firstFixTimer =
      g_timeout_add(static_cast<guint>(m_firstFixTimeout.count()), &GeoClueLocationService::OnFirstFixTimeout, this);
  m_cancellable = g_cancellable_new();
#ifdef GEOCLUE_PORTAL
  // Match GClueSimple's portal selection; the native client must be observed before its first fix.
  char const * portal = g_getenv("GTK_USE_PORTAL");
  if (g_file_test("/.flatpak-info", G_FILE_TEST_EXISTS) || (portal && portal[0] == '1'))
  {
    gclue_simple_new(m_desktopId.c_str(), GCLUE_ACCURACY_LEVEL_EXACT, m_cancellable,
                     &GeoClueLocationService::OnSimpleReady, NewRequest());
    return;
  }
#endif
  gclue_client_proxy_create_full(m_desktopId.c_str(), GCLUE_ACCURACY_LEVEL_EXACT, GCLUE_CLIENT_PROXY_CREATE_AUTO_DELETE,
                                 m_cancellable, &GeoClueLocationService::OnClientReady, NewRequest());
}

void GeoClueLocationService::Stop()
{
  if (!m_started)
    return;
  m_started = false;
  g_clear_handle_id(&m_firstFixTimer, g_source_remove);
  g_cancellable_cancel(m_cancellable);
  g_clear_object(&m_cancellable);
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
}

void GeoClueLocationService::Fail(TLocationError error)
{
  // Stop first: the observer may restart the service.
  Stop();
  m_observer.OnLocationError(error);
}

void GeoClueLocationService::LocationError(GError * error)
{
  LOG(LWARNING, ("GeoClue failed:", error->message));
  TLocationError code = EUnknown;
  if (g_error_matches(error, G_DBUS_ERROR, G_DBUS_ERROR_ACCESS_DENIED))
    code = EDenied;
  else if (g_error_matches(error, G_DBUS_ERROR, G_DBUS_ERROR_SERVICE_UNKNOWN) ||
           g_error_matches(error, G_DBUS_ERROR, G_DBUS_ERROR_NAME_HAS_NO_OWNER))
    code = ENotSupported;
  // GClueSimple reports a portal denial as G_IO_ERROR_FAILED, like its other failures.
  g_error_free(error);
  Fail(code);
}

gboolean GeoClueLocationService::OnFirstFixTimeout(gpointer data)
{
  auto * self = static_cast<GeoClueLocationService *>(data);
  ASSERT(self->m_started, ());
  // The source is removed on return.
  self->m_firstFixTimer = 0;
  LOG(LWARNING, ("No location from GeoClue in", self->m_firstFixTimeout.count(), "ms"));
  self->Fail(ETimeout);
  return G_SOURCE_REMOVE;
}

void GeoClueLocationService::OnClientReady(GObject *, GAsyncResult * res, gpointer data)
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
  g_signal_connect(client, "notify::active", G_CALLBACK(&GeoClueLocationService::OnClientActiveNotify), self);
  g_signal_connect(client, "location-updated", G_CALLBACK(&GeoClueLocationService::OnLocationUpdated), self);
  gclue_client_call_start(client, self->m_cancellable, &GeoClueLocationService::OnClientStarted, self->NewRequest());
}

void GeoClueLocationService::OnClientStarted(GObject * source, GAsyncResult * res, gpointer data)
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
  // GeoClue may return Start before its queued PropertiesChanged reaches the proxy cache.
  g_dbus_proxy_call(G_DBUS_PROXY(source), "org.freedesktop.DBus.Properties.GetAll",
                    g_variant_new("(s)", "org.freedesktop.GeoClue2.Client"), G_DBUS_CALL_FLAGS_NONE, -1,
                    self->m_cancellable, &GeoClueLocationService::OnClientPropertiesReady, self->NewRequest());
}

void GeoClueLocationService::OnClientPropertiesReady(GObject * source, GAsyncResult * res, gpointer data)
{
  GError * error = nullptr;
  GVariant * result = g_dbus_proxy_call_finish(G_DBUS_PROXY(source), res, &error);
  auto * self = TakeRequest(data);
  if (!self)
  {
    g_clear_pointer(&result, g_variant_unref);
    g_clear_error(&error);
    return;
  }
  if (!result)
  {
    self->LocationError(error);
    return;
  }
  GVariant * properties = nullptr;
  g_variant_get(result, "(@a{sv})", &properties);
  gboolean active = FALSE;
  g_variant_lookup(properties, "Active", "b", &active);
  char const * path = nullptr;
  // A LocationUpdated received since Start has already requested a newer position.
  if (self->CheckClientActive(active) && !self->m_locationRequest &&
      g_variant_lookup(properties, "Location", "&o", &path))
    self->LoadLocation(path);
  g_variant_unref(properties);
  g_variant_unref(result);
}

void GeoClueLocationService::OnLocationUpdated(GClueClient *, char const *, char const * newPath, gpointer data)
{
  auto * self = static_cast<GeoClueLocationService *>(data);
  if (self->m_clientStarted)
    self->LoadLocation(newPath);
}

void GeoClueLocationService::LoadLocation(char const * path)
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
                                   m_locationRequest, &GeoClueLocationService::OnLocationReady,
                                   NewRequest(m_locationRequest));
}

void GeoClueLocationService::OnLocationReady(GObject *, GAsyncResult * res, gpointer data)
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

void GeoClueLocationService::OnSimpleReady(GObject *, GAsyncResult * res, gpointer data)
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
  // Don't cancel the finished init: GClueSimple still listens to its cancellable and would return the task twice.
  g_clear_object(&self->m_cancellable);
  if (!simple)
  {
    self->LocationError(error);
    return;
  }
  self->m_simple = simple;
  g_signal_connect(simple, "notify::location", G_CALLBACK(&GeoClueLocationService::OnLocationNotify), self);
  self->ReadLocation(gclue_simple_get_location(simple));
}

void GeoClueLocationService::OnClientActiveNotify(GObject * obj, GParamSpec *, gpointer data)
{
  static_cast<GeoClueLocationService *>(data)->CheckClientActive(gclue_client_get_active(GCLUE_CLIENT(obj)));
}

bool GeoClueLocationService::CheckClientActive(bool active)
{
  // Active=false is normal until Client.Start has completed.
  if (!m_clientStarted || active)
    return true;
  LOG(LWARNING, ("GeoClue deactivated the client: location is turned off"));
  Fail(EGPSIsOff);
  return false;
}

void GeoClueLocationService::OnLocationNotify(GObject *, GParamSpec *, gpointer data)
{
  auto * self = static_cast<GeoClueLocationService *>(data);
  self->ReadLocation(gclue_simple_get_location(self->m_simple));
}

void GeoClueLocationService::ReadLocation(GClueLocation * loc)
{
  ASSERT(m_started, ());
  if (!loc)
    return;

  GpsInfo info;
  info.m_source = EGeoClue2;
  info.m_latitude = gclue_location_get_latitude(loc);
  info.m_longitude = gclue_location_get_longitude(loc);
  info.m_horizontalAccuracy = gclue_location_get_accuracy(loc);
  if (info.m_horizontalAccuracy <= 0)
    info.m_horizontalAccuracy = 100;
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
  // Suppress stationary headings. A moving fix may still carry a compass heading, not the course over ground.
  if (heading >= 0 && speed > 0.5)
    info.m_bearing = heading;

  // A proxy for an unavailable location object has no timestamp; zero makes the filter reject it.
  if (GVariant * ts = gclue_location_get_timestamp(loc))
  {
    guint64 sec = 0, usec = 0;
    g_variant_get(ts, "(tt)", &sec, &usec);
    info.m_timestamp = static_cast<double>(sec) + static_cast<double>(usec) / 1e6;
  }

  if (!m_filter.Passes(info))
  {
    LOG(LINFO, ("Skipping an outdated location, timestamp", info.m_timestamp));
    return;
  }
  g_clear_handle_id(&m_firstFixTimer, g_source_remove);
  m_observer.OnLocationUpdated(info);
}
}  // namespace location
