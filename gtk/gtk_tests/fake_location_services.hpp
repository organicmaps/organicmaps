#pragma once

#include "gtk/location.hpp"

#include "base/assert.hpp"

#include <gio/gio.h>

#include <algorithm>
#include <string>
#include <vector>

namespace gtk_test_support
{
char constexpr kXml[] = R"xml(
<node>
  <interface name="org.freedesktop.DBus.Properties">
    <method name="GetAll"><arg type="s" direction="in"/><arg type="a{sv}" direction="out"/></method>
  </interface>
  <interface name="org.freedesktop.GeoClue2.Manager">
    <method name="GetClient"><arg name="client" type="o" direction="out"/></method>
    <method name="CreateClient"><arg name="client" type="o" direction="out"/></method>
    <method name="DeleteClient"><arg name="client" type="o" direction="in"/></method>
    <property name="InUse" type="b" access="read"/>
    <property name="AvailableAccuracyLevel" type="u" access="read"/>
  </interface>
  <interface name="org.freedesktop.GeoClue2.Client">
    <method name="Start"/>
    <method name="Stop"/>
    <property name="Location" type="o" access="read"/>
    <property name="DistanceThreshold" type="u" access="readwrite"/>
    <property name="TimeThreshold" type="u" access="readwrite"/>
    <property name="DesktopId" type="s" access="readwrite"/>
    <property name="RequestedAccuracyLevel" type="u" access="readwrite"/>
    <property name="Active" type="b" access="read"/>
    <signal name="LocationUpdated"><arg name="old" type="o"/><arg name="new" type="o"/></signal>
  </interface>
  <interface name="org.freedesktop.GeoClue2.Location">
    <property name="Latitude" type="d" access="read"/>
    <property name="Longitude" type="d" access="read"/>
    <property name="Accuracy" type="d" access="read"/>
    <property name="Altitude" type="d" access="read"/>
    <property name="Speed" type="d" access="read"/>
    <property name="Heading" type="d" access="read"/>
    <property name="Description" type="s" access="read"/>
    <property name="Timestamp" type="(tt)" access="read"/>
  </interface>
  <interface name="net.hadess.SensorProxy.Compass">
    <method name="ClaimCompass"/>
    <method name="ReleaseCompass"/>
    <property name="HasCompass" type="b" access="read"/>
    <property name="CompassHeading" type="d" access="read"/>
  </interface>
  <interface name="org.freedesktop.portal.Location">
    <method name="CreateSession"><arg type="a{sv}" direction="in"/><arg type="o" direction="out"/></method>
    <method name="Start"><arg type="o" direction="in"/><arg type="s" direction="in"/>
      <arg type="a{sv}" direction="in"/><arg type="o" direction="out"/></method>
    <signal name="LocationUpdated"><arg type="o"/><arg type="a{sv}"/></signal>
    <property name="version" type="u" access="read"/>
  </interface>
  <interface name="org.freedesktop.portal.Session"><method name="Close"/></interface>
</node>)xml";

char constexpr kClientPath[] = "/org/freedesktop/GeoClue2/Client/1";
char constexpr kLocationPath[] = "/org/freedesktop/GeoClue2/Client/1/Location/1";
char constexpr kSecondLocationPath[] = "/org/freedesktop/GeoClue2/Client/1/Location/2";
char constexpr kCompassPath[] = "/net/hadess/SensorProxy/Compass";

// Runs the default main context, which serves both the services and the tested code.
template <typename Pred>
bool RunUntil(Pred && pred, int timeoutMs = 5000)
{
  gint64 const deadline = g_get_monotonic_time() + timeoutMs * 1000LL;
  guint const tick = g_timeout_add(10, [](gpointer) -> gboolean { return G_SOURCE_CONTINUE; }, nullptr);
  while (!pred() && g_get_monotonic_time() < deadline)
    g_main_context_iteration(nullptr, TRUE);
  g_source_remove(tick);
  return pred();
}

inline void RunFor(int ms)
{
  gint64 const end = g_get_monotonic_time() + ms * 1000;
  RunUntil([end]() { return g_get_monotonic_time() >= end; });
}

// GeoClue and iio-sensor-proxy on a private system bus. Client creation, location proxy setup and
// compass claims may be held to complete after the service has been stopped.
class FakeServices
{
public:
  explicit FakeServices(bool privateBus = true)
  {
    char const * address = g_getenv("DBUS_SESSION_BUS_ADDRESS");
    if (privateBus)
    {
      m_bus = g_test_dbus_new(G_TEST_DBUS_NONE);
      g_test_dbus_up(m_bus);
      address = g_test_dbus_get_bus_address(m_bus);
    }
    CHECK(address, ("dbus-daemon is required"));
    g_setenv("DBUS_SYSTEM_BUS_ADDRESS", address, TRUE);
    // A real GtkApplication keeps the session connection alive while portal sessions close.
    if (privateBus)
      m_session = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
    // The tested code uses this singleton. Closing the test bus must not exit the process.
    m_system = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, nullptr);
    g_dbus_connection_set_exit_on_close(m_system, FALSE);

    m_conn =
        g_dbus_connection_new_for_address_sync(address,
                                               GDBusConnectionFlags(G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
                                                                    G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION),
                                               nullptr, nullptr, nullptr);
    m_node = g_dbus_node_info_new_for_xml(kXml, nullptr);
    Register("/org/freedesktop/GeoClue2/Manager", "org.freedesktop.GeoClue2.Manager");
    Register(kClientPath, "org.freedesktop.GeoClue2.Client");
    for (char const * path : {kLocationPath, kSecondLocationPath})
    {
      Register(path, "org.freedesktop.GeoClue2.Location");
      // Override automatic Properties replies so GetAll can complete asynchronously.
      Register(path, "org.freedesktop.DBus.Properties");
    }
    Register(kCompassPath, "net.hadess.SensorProxy.Compass");
    Register("/org/freedesktop/portal/desktop", "org.freedesktop.portal.Location");
    for (char const * name : {"org.freedesktop.GeoClue2", "net.hadess.SensorProxy", "org.freedesktop.portal.Desktop"})
    {
      GVariant * reply = g_dbus_connection_call_sync(
          m_conn, "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "RequestName",
          g_variant_new("(su)", name, 4u /* DO_NOT_QUEUE */), nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);
      g_variant_unref(reply);
    }
  }

  ~FakeServices()
  {
    Answer();
    for (guint id : m_registrations)
      g_dbus_connection_unregister_object(m_conn, id);
    g_dbus_node_info_unref(m_node);
    g_object_unref(m_conn);
    g_dbus_connection_close_sync(m_system, nullptr, nullptr);
    g_object_unref(m_system);
    if (m_session)
    {
      g_dbus_connection_close_sync(m_session, nullptr, nullptr);
      g_object_unref(m_session);
    }
    if (m_bus)
    {
      g_test_dbus_down(m_bus);
      g_object_unref(m_bus);
    }
    g_unsetenv("DBUS_SYSTEM_BUS_ADDRESS");
  }

  void Hold(bool geoClue, bool compass)
  {
    m_holdGeoClue = geoClue;
    m_holdCompass = compass;
  }

  // Answers the held calls, also those the caller has cancelled.
  void Answer()
  {
    Hold(false, false);
    m_holdLocation = false;
    for (auto * invocation : m_held)
      Reply(invocation);
    m_held.clear();
  }

  // Forgets the calls and the published values of the previous case.
  void Reset()
  {
    m_calls.clear();
    m_locationPath = "/";
    m_heading = -1;
    m_active = false;
    m_holdStart = false;
    m_holdLocation = false;
    m_deactivateOnStart = false;
    m_hasCompass = true;
    m_denyLocation = false;
    m_failCompassClaim = false;
  }

  size_t HeldCount() const { return m_held.size(); }
  std::vector<std::string> const & Calls() const { return m_calls; }
  int Count(std::string const & method) const { return static_cast<int>(std::ranges::count(m_calls, method)); }
  // The position of the first call, or the number of calls.
  int Index(std::string const & method) const
  {
    return static_cast<int>(std::ranges::find(m_calls, method) - m_calls.begin());
  }

  void PublishPortalFix()
  {
    GVariantBuilder values;
    g_variant_builder_init(&values, G_VARIANT_TYPE_VARDICT);
    for (char const * prop :
         {"Latitude", "Longitude", "Accuracy", "Altitude", "Speed", "Heading", "Description", "Timestamp"})
      g_variant_builder_add(&values, "{sv}", prop,
                            OnGetProperty(nullptr, nullptr, nullptr, nullptr, prop, nullptr, this));
    g_dbus_connection_emit_signal(m_conn, nullptr, "/org/freedesktop/portal/desktop", "org.freedesktop.portal.Location",
                                  "LocationUpdated", g_variant_new("(oa{sv})", m_portalSession.c_str(), &values),
                                  nullptr);
  }

  void HoldLocation(bool hold) { m_holdLocation = hold; }

  void PublishFix(char const * path = kLocationPath)
  {
    m_locationPath = path;
    g_dbus_connection_emit_signal(m_conn, nullptr, kClientPath, "org.freedesktop.GeoClue2.Client", "LocationUpdated",
                                  g_variant_new("(oo)", "/", path), nullptr);
  }

  void SetActive(bool active)
  {
    m_active = active;
    GVariantBuilder changed;
    g_variant_builder_init(&changed, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&changed, "{sv}", "Active", g_variant_new_boolean(active));
    g_dbus_connection_emit_signal(m_conn, nullptr, kClientPath, "org.freedesktop.DBus.Properties", "PropertiesChanged",
                                  g_variant_new("(sa{sv}as)", "org.freedesktop.GeoClue2.Client", &changed, nullptr),
                                  nullptr);
  }

  void HoldStart(bool hold) { m_holdStart = hold; }
  void DeactivateOnStart(bool deactivate) { m_deactivateOnStart = deactivate; }

  void SetCompassServiceAvailable(bool available)
  {
    GVariant * reply = g_dbus_connection_call_sync(m_conn, "org.freedesktop.DBus", "/org/freedesktop/DBus",
                                                   "org.freedesktop.DBus", available ? "RequestName" : "ReleaseName",
                                                   available ? g_variant_new("(su)", "net.hadess.SensorProxy", 4u)
                                                             : g_variant_new("(s)", "net.hadess.SensorProxy"),
                                                   nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);
    CHECK(reply, ());
    g_variant_unref(reply);
  }

  void DenyLocation(bool denied) { m_denyLocation = denied; }
  void FailCompassClaim(bool fail) { m_failCompassClaim = fail; }

  void SetHasCompass(bool available)
  {
    m_hasCompass = available;
    GVariantBuilder changed;
    g_variant_builder_init(&changed, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&changed, "{sv}", "HasCompass", g_variant_new_boolean(available));
    g_dbus_connection_emit_signal(m_conn, nullptr, kCompassPath, "org.freedesktop.DBus.Properties", "PropertiesChanged",
                                  g_variant_new("(sa{sv}as)", "net.hadess.SensorProxy.Compass", &changed, nullptr),
                                  nullptr);
  }

  void SetHeading(double degrees, bool notify = true)
  {
    m_heading = degrees;
    if (!notify)
      return;
    GVariantBuilder changed;
    g_variant_builder_init(&changed, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&changed, "{sv}", "CompassHeading", g_variant_new_double(degrees));
    g_dbus_connection_emit_signal(m_conn, nullptr, kCompassPath, "org.freedesktop.DBus.Properties", "PropertiesChanged",
                                  g_variant_new("(sa{sv}as)", "net.hadess.SensorProxy.Compass", &changed, nullptr),
                                  nullptr);
  }

private:
  void Register(char const * path, char const * iface)
  {
    static GDBusInterfaceVTable const vtable = {
        &FakeServices::OnMethodCall, &FakeServices::OnGetProperty, &FakeServices::OnSetProperty, {}};
    m_registrations.push_back(g_dbus_connection_register_object(
        m_conn, path, g_dbus_node_info_lookup_interface(m_node, iface), &vtable, this, nullptr, nullptr));
  }

  void Reply(GDBusMethodInvocation * invocation)
  {
    std::string const method = g_dbus_method_invocation_get_method_name(invocation);
    if (method == "GetAll")
    {
      GVariantBuilder values;
      g_variant_builder_init(&values, G_VARIANT_TYPE_VARDICT);
      for (char const * prop :
           {"Latitude", "Longitude", "Accuracy", "Altitude", "Speed", "Heading", "Description", "Timestamp"})
        g_variant_builder_add(&values, "{sv}", prop,
                              OnGetProperty(nullptr, nullptr, nullptr, nullptr, prop, nullptr, this));
      g_dbus_method_invocation_return_value(invocation, g_variant_new("(a{sv})", &values));
    }
    else if (method == "GetClient" || method == "CreateClient")
      g_dbus_method_invocation_return_value(invocation, g_variant_new("(o)", kClientPath));
    else
    {
      if (method == "Start" && !m_deactivateOnStart)
        SetActive(true);
      if (method == "Stop")
        SetActive(false);
      g_dbus_method_invocation_return_value(invocation, nullptr);
    }
  }

  static void OnMethodCall(GDBusConnection *, char const * sender, char const *, char const * iface,
                           char const * method, GVariant * args, GDBusMethodInvocation * invocation, gpointer data)
  {
    auto * self = static_cast<FakeServices *>(data);
    std::string const name = method;
    self->m_calls.push_back(name);
    if (g_str_equal(iface, "org.freedesktop.portal.Location"))
    {
      GVariant * options = g_variant_get_child_value(args, name == "CreateSession" ? 0 : 2);
      char const * token = nullptr;
      g_variant_lookup(options, name == "CreateSession" ? "session_handle_token" : "handle_token", "&s", &token);
      CHECK(token, ());
      std::string caller = sender + 1;
      std::ranges::replace(caller, '.', '_');
      std::string const path = std::string("/org/freedesktop/portal/desktop/") +
                               (name == "CreateSession" ? "session/" : "request/") + caller + "/" + token;
      if (name == "CreateSession")
      {
        self->m_portalSession = path;
        self->Register(path.c_str(), "org.freedesktop.portal.Session");
      }
      g_dbus_method_invocation_return_value(invocation, g_variant_new("(o)", path.c_str()));
      if (name == "Start")
        g_dbus_connection_emit_signal(self->m_conn, sender, path.c_str(), "org.freedesktop.portal.Request", "Response",
                                      g_variant_new("(ua{sv})", 0u, nullptr), nullptr);
      g_variant_unref(options);
      return;
    }
    if ((name == "Start" && self->m_denyLocation) || (name == "ClaimCompass" && self->m_failCompassClaim))
    {
      g_dbus_method_invocation_return_dbus_error(invocation, "org.freedesktop.DBus.Error.AccessDenied",
                                                 "Denied by test");
      return;
    }
    bool const createsClient = name == "GetClient" || name == "CreateClient";
    if ((createsClient && self->m_holdGeoClue) || (name == "ClaimCompass" && self->m_holdCompass) ||
        (name == "Start" && self->m_holdStart) || (name == "GetAll" && self->m_holdLocation))
      self->m_held.push_back(invocation);
    else
      self->Reply(invocation);
  }

  static GVariant * OnGetProperty(GDBusConnection *, char const *, char const *, char const *, char const * prop,
                                  GError **, gpointer data)
  {
    auto const * self = static_cast<FakeServices *>(data);
    std::string const name = prop;
    if (name == "Active")
      return g_variant_new_boolean(self->m_active);
    if (name == "HasCompass")
      return g_variant_new_boolean(self->m_hasCompass);
    if (name == "InUse")
      return g_variant_new_boolean(TRUE);
    if (name == "version")
      return g_variant_new_uint32(1);
    if (name == "AvailableAccuracyLevel")
      return g_variant_new_uint32(8);
    if (name == "DistanceThreshold" || name == "TimeThreshold" || name == "RequestedAccuracyLevel")
      return g_variant_new_uint32(0);
    if (name == "DesktopId" || name == "Description")
      return g_variant_new_string("");
    if (name == "Location")
      return g_variant_new_object_path(self->m_locationPath.c_str());
    if (name == "Timestamp")
    {
      gint64 const now = g_get_real_time();
      return g_variant_new("(tt)", guint64(now / G_USEC_PER_SEC), guint64(now % G_USEC_PER_SEC));
    }
    if (name == "Latitude")
      return g_variant_new_double(52.52);
    if (name == "Longitude")
      return g_variant_new_double(13.40);
    if (name == "Accuracy")
      return g_variant_new_double(10);
    if (name == "Altitude")
      return g_variant_new_double(-G_MAXDOUBLE);
    if (name == "CompassHeading")
      return g_variant_new_double(self->m_heading);
    return g_variant_new_double(-1);  // Speed, Heading
  }

  static gboolean OnSetProperty(GDBusConnection *, char const *, char const *, char const *, char const *, GVariant *,
                                GError **, gpointer)
  {
    return TRUE;
  }

  GTestDBus * m_bus = nullptr;
  GDBusConnection * m_system = nullptr;
  GDBusConnection * m_session = nullptr;
  GDBusConnection * m_conn = nullptr;
  GDBusNodeInfo * m_node = nullptr;
  std::vector<guint> m_registrations;
  std::vector<GDBusMethodInvocation *> m_held;
  std::vector<std::string> m_calls;
  bool m_holdGeoClue = false;
  bool m_holdCompass = false;
  bool m_active = false;
  bool m_holdStart = false;
  bool m_holdLocation = false;
  bool m_deactivateOnStart = false;
  bool m_hasCompass = true;
  bool m_denyLocation = false;
  bool m_failCompassClaim = false;
  double m_heading = -1;
  std::string m_locationPath = "/";
  std::string m_portalSession;
};

struct Counters
{
  int m_locations = 0;
  int m_errors = 0;
  int m_headings = 0;
  location::TLocationError m_lastError = location::ENoError;
  double m_lastHeading = -1;
};

inline void Watch(gtk::LocationService & service, Counters & counters)
{
  service.m_onLocation = [&counters](location::GpsInfo const &) { ++counters.m_locations; };
  service.m_onError = [&counters](location::TLocationError error)
  {
    ++counters.m_errors;
    counters.m_lastError = error;
  };
  service.m_onCompass = [&counters](double heading)
  {
    ++counters.m_headings;
    counters.m_lastHeading = heading;
  };
}

}  // namespace gtk_test_support
