#pragma once

#include "base/assert.hpp"

#include <gio/gio.h>

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace location::test
{
char constexpr kXml[] = R"xml(
<node>
  <interface name="org.freedesktop.DBus.Properties">
    <method name="Get"><arg type="s" direction="in"/><arg type="s" direction="in"/><arg type="v" direction="out"/></method>
    <method name="GetAll"><arg type="s" direction="in"/><arg type="a{sv}" direction="out"/></method>
    <method name="Set"><arg type="s" direction="in"/><arg type="s" direction="in"/><arg type="v" direction="in"/></method>
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
  <interface name="org.freedesktop.portal.Location">
    <method name="CreateSession"><arg type="a{sv}" direction="in"/><arg type="o" direction="out"/></method>
    <method name="Start"><arg type="o" direction="in"/><arg type="s" direction="in"/>
      <arg type="a{sv}" direction="in"/><arg type="o" direction="out"/></method>
    <signal name="LocationUpdated"><arg type="o"/><arg type="a{sv}"/></signal>
    <property name="version" type="u" access="read"/>
  </interface>
  <interface name="org.freedesktop.portal.Session"><method name="Close"/></interface>
</node>)xml";

char constexpr kGeoClueName[] = "org.freedesktop.GeoClue2";
char constexpr kClientPath[] = "/org/freedesktop/GeoClue2/Client/1";
char constexpr kLocationPath[] = "/org/freedesktop/GeoClue2/Client/1/Location/1";
char constexpr kSecondLocationPath[] = "/org/freedesktop/GeoClue2/Client/1/Location/2";
double constexpr kLatitude = 52.52;
double constexpr kLongitude = 13.40;
double constexpr kAccuracy = 10;

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

// GeoClue and the location portal on a private bus that serves as both the system and the session bus.
// Client creation, Client.Start, the client and location properties may be held to complete later,
// e.g. after the service has been stopped.
class FakeGeoClueServices
{
public:
  FakeGeoClueServices()
  {
    m_bus = g_test_dbus_new(G_TEST_DBUS_NONE);
    g_test_dbus_up(m_bus);
    char const * address = g_test_dbus_get_bus_address(m_bus);
    CHECK(address, ("dbus-daemon is required"));
    g_setenv("DBUS_SYSTEM_BUS_ADDRESS", address, TRUE);
    // A real app keeps the session connection alive while portal sessions close.
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
    Register(kClientPath, "org.freedesktop.DBus.Properties");
    for (char const * path : {kLocationPath, kSecondLocationPath})
    {
      Register(path, "org.freedesktop.GeoClue2.Location");
      // Override automatic Properties replies so GetAll can complete asynchronously.
      Register(path, "org.freedesktop.DBus.Properties");
    }
    Register("/org/freedesktop/portal/desktop", "org.freedesktop.portal.Location");
    SetGeoClueAvailable(true);
    CallBus("RequestName", g_variant_new("(su)", "org.freedesktop.portal.Desktop", 4u /* DO_NOT_QUEUE */));
  }

  ~FakeGeoClueServices()
  {
    Answer();
    for (guint id : m_registrations)
      g_dbus_connection_unregister_object(m_conn, id);
    g_dbus_node_info_unref(m_node);
    g_object_unref(m_conn);
    g_dbus_connection_close_sync(m_system, nullptr, nullptr);
    g_object_unref(m_system);
    g_dbus_connection_close_sync(m_session, nullptr, nullptr);
    g_object_unref(m_session);
    g_test_dbus_down(m_bus);
    g_object_unref(m_bus);
    g_unsetenv("DBUS_SYSTEM_BUS_ADDRESS");
  }

  void HoldClientCreation(bool hold) { m_holdClientCreation = hold; }

  // Answers the held calls, also those the caller has cancelled.
  void Answer()
  {
    m_holdClientCreation = false;
    m_holdLocation = false;
    m_holdClientProperties = false;
    for (auto * invocation : m_held)
      Reply(invocation);
    m_held.clear();
  }

  // Forgets the calls and the published values of the previous case.
  void Reset()
  {
    m_calls.clear();
    m_locationPath = "/";
    m_active = false;
    m_holdStart = false;
    m_holdLocation = false;
    m_holdClientProperties = false;
    m_clientHasStarted = false;
    m_delayStartNotifications = false;
    m_publishLocationDuringStart = false;
    m_deactivateOnStart = false;
    m_startError.clear();
    m_portalResponse = 0;
    m_fixAgeSec = 0;
    m_fixTime.reset();
    m_fixAccuracy = kAccuracy;
    m_fixAltitude = -G_MAXDOUBLE;
  }

  size_t HeldCount() const { return m_held.size(); }
  std::vector<std::string> const & Calls() const { return m_calls; }
  int Count(std::string const & method) const { return static_cast<int>(std::ranges::count(m_calls, method)); }
  std::string const & DesktopId() const { return m_desktopId; }

  void PublishPortalFix()
  {
    g_dbus_connection_emit_signal(m_conn, nullptr, "/org/freedesktop/portal/desktop", "org.freedesktop.portal.Location",
                                  "LocationUpdated",
                                  g_variant_new("(o@a{sv})", m_portalSession.c_str(), LocationProperties()), nullptr);
  }

  void HoldLocation(bool hold) { m_holdLocation = hold; }
  void HoldClientProperties(bool hold) { m_holdClientProperties = hold; }
  void DelayStartNotifications(bool delay) { m_delayStartNotifications = delay; }
  void PublishLocationDuringStart(bool publish) { m_publishLocationDuringStart = publish; }

  void PublishFix(char const * path = kLocationPath)
  {
    m_locationPath = path;
    g_dbus_connection_emit_signal(m_conn, nullptr, kClientPath, "org.freedesktop.GeoClue2.Client", "LocationUpdated",
                                  g_variant_new("(oo)", "/", path), nullptr);
  }

  void SetFixAge(guint64 seconds) { m_fixAgeSec = seconds; }
  void SetFixTime(guint64 sec, guint64 usec) { m_fixTime = {sec, usec}; }
  void SetFixAccuracy(double accuracy) { m_fixAccuracy = accuracy; }
  void SetFixAltitude(double altitude) { m_fixAltitude = altitude; }

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
  // A D-Bus error name for Client.Start, empty for success.
  void FailStart(std::string const & error) { m_startError = error; }
  // 0 is success, 1 is a user denial, 2 is another failure.
  void SetPortalResponse(guint response) { m_portalResponse = response; }

  void SetGeoClueAvailable(bool available)
  {
    if (available)
      CallBus("RequestName", g_variant_new("(su)", kGeoClueName, 4u /* DO_NOT_QUEUE */));
    else
      CallBus("ReleaseName", g_variant_new("(s)", kGeoClueName));
  }

private:
  void CallBus(char const * method, GVariant * args)
  {
    GVariant * reply =
        g_dbus_connection_call_sync(m_conn, "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
                                    method, args, nullptr, G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr);
    CHECK(reply, (method));
    g_variant_unref(reply);
  }

  void Register(char const * path, char const * iface)
  {
    static GDBusInterfaceVTable const vtable = {&FakeGeoClueServices::OnMethodCall,
                                                &FakeGeoClueServices::OnGetProperty,
                                                &FakeGeoClueServices::OnSetProperty,
                                                {}};
    m_registrations.push_back(g_dbus_connection_register_object(
        m_conn, path, g_dbus_node_info_lookup_interface(m_node, iface), &vtable, this, nullptr, nullptr));
  }

  GVariant * LocationProperties()
  {
    GVariantBuilder values;
    g_variant_builder_init(&values, G_VARIANT_TYPE_VARDICT);
    for (char const * prop :
         {"Latitude", "Longitude", "Accuracy", "Altitude", "Speed", "Heading", "Description", "Timestamp"})
      g_variant_builder_add(&values, "{sv}", prop,
                            OnGetProperty(nullptr, nullptr, nullptr, nullptr, prop, nullptr, this));
    return g_variant_builder_end(&values);
  }

  void Reply(GDBusMethodInvocation * invocation)
  {
    std::string const method = g_dbus_method_invocation_get_method_name(invocation);
    if (g_str_equal(g_dbus_method_invocation_get_interface_name(invocation), "org.freedesktop.portal.Location"))
      ReplyPortal(invocation);
    else if (method == "Get")
    {
      char const * iface = nullptr;
      char const * prop = nullptr;
      g_variant_get(g_dbus_method_invocation_get_parameters(invocation), "(&s&s)", &iface, &prop);
      g_dbus_method_invocation_return_value(
          invocation, g_variant_new("(v)", OnGetProperty(nullptr, nullptr, nullptr, iface, prop, nullptr, this)));
    }
    else if (method == "GetAll")
    {
      if (g_str_equal(g_dbus_method_invocation_get_object_path(invocation), kClientPath))
      {
        GVariantBuilder values;
        g_variant_builder_init(&values, G_VARIANT_TYPE_VARDICT);
        for (char const * prop : {"Active", "Location"})
          g_variant_builder_add(&values, "{sv}", prop,
                                OnGetProperty(nullptr, nullptr, nullptr, nullptr, prop, nullptr, this));
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(a{sv})", &values));
      }
      else
      {
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(@a{sv})", LocationProperties()));
      }
    }
    else if (method == "GetClient" || method == "CreateClient")
    {
      g_dbus_method_invocation_return_value(invocation, g_variant_new("(o)", kClientPath));
    }
    else
    {
      if (method == "Start")
      {
        m_clientHasStarted = true;
        if (!m_deactivateOnStart)
        {
          if (m_delayStartNotifications)
            m_active = true;
          else
            SetActive(true);
        }
        if (m_publishLocationDuringStart)
          PublishFix();
      }
      if (method == "Stop")
        SetActive(false);
      g_dbus_method_invocation_return_value(invocation, nullptr);
    }
  }

  void ReplyPortal(GDBusMethodInvocation * invocation)
  {
    std::string const name = g_dbus_method_invocation_get_method_name(invocation);
    GVariant * options =
        g_variant_get_child_value(g_dbus_method_invocation_get_parameters(invocation), name == "CreateSession" ? 0 : 2);
    char const * token = nullptr;
    g_variant_lookup(options, name == "CreateSession" ? "session_handle_token" : "handle_token", "&s", &token);
    CHECK(token, ());
    // Returning a value frees the invocation.
    std::string const sender = g_dbus_method_invocation_get_sender(invocation);
    std::string caller = sender.substr(1);
    std::ranges::replace(caller, '.', '_');
    std::string const path = std::string("/org/freedesktop/portal/desktop/") +
                             (name == "CreateSession" ? "session/" : "request/") + caller + "/" + token;
    if (name == "CreateSession")
    {
      m_portalSession = path;
      Register(path.c_str(), "org.freedesktop.portal.Session");
    }
    g_dbus_method_invocation_return_value(invocation, g_variant_new("(o)", path.c_str()));
    if (name == "Start")
    {
      g_dbus_connection_emit_signal(m_conn, sender.c_str(), path.c_str(), "org.freedesktop.portal.Request", "Response",
                                    g_variant_new("(ua{sv})", m_portalResponse, nullptr), nullptr);
    }
    g_variant_unref(options);
  }

  static void OnMethodCall(GDBusConnection *, char const *, char const *, char const * iface, char const * method,
                           GVariant * args, GDBusMethodInvocation * invocation, gpointer data)
  {
    auto * self = static_cast<FakeGeoClueServices *>(data);
    std::string const name = method;
    self->m_calls.push_back(name);
    if (g_str_equal(iface, "org.freedesktop.portal.Location"))
    {
      if (name == "Start" && self->m_holdStart)
        self->m_held.push_back(invocation);
      else
        self->ReplyPortal(invocation);
      return;
    }
    if (name == "Set")
    {
      char const * prop = nullptr;
      GVariant * value = nullptr;
      g_variant_get(args, "(&s&sv)", nullptr, &prop, &value);
      if (g_str_equal(prop, "DesktopId"))
        self->m_desktopId = g_variant_get_string(value, nullptr);
      g_variant_unref(value);
    }
    if (name == "Start" && !self->m_startError.empty())
    {
      g_dbus_method_invocation_return_dbus_error(invocation, self->m_startError.c_str(), "Failed by test");
      return;
    }
    bool const createsClient = name == "GetClient" || name == "CreateClient";
    bool const clientProperties = g_str_equal(g_dbus_method_invocation_get_object_path(invocation), kClientPath);
    if ((createsClient && self->m_holdClientCreation) || (name == "Start" && self->m_holdStart) ||
        (name == "GetAll" &&
         (clientProperties ? self->m_holdClientProperties && self->m_clientHasStarted : self->m_holdLocation)))
      self->m_held.push_back(invocation);
    else
      self->Reply(invocation);
  }

  static GVariant * OnGetProperty(GDBusConnection *, char const *, char const *, char const *, char const * prop,
                                  GError **, gpointer data)
  {
    auto const * self = static_cast<FakeGeoClueServices *>(data);
    std::string const name = prop;
    if (name == "Active")
      return g_variant_new_boolean(self->m_active);
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
      if (self->m_fixTime)
        return g_variant_new("(tt)", self->m_fixTime->first, self->m_fixTime->second);
      gint64 const now = g_get_real_time();
      return g_variant_new("(tt)", guint64(now / G_USEC_PER_SEC) - self->m_fixAgeSec, guint64(now % G_USEC_PER_SEC));
    }
    if (name == "Latitude")
      return g_variant_new_double(kLatitude);
    if (name == "Longitude")
      return g_variant_new_double(kLongitude);
    if (name == "Accuracy")
      return g_variant_new_double(self->m_fixAccuracy);
    if (name == "Altitude")
      return g_variant_new_double(self->m_fixAltitude);
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
  bool m_holdClientCreation = false;
  bool m_active = false;
  bool m_holdStart = false;
  bool m_holdLocation = false;
  bool m_holdClientProperties = false;
  bool m_clientHasStarted = false;
  bool m_delayStartNotifications = false;
  bool m_publishLocationDuringStart = false;
  bool m_deactivateOnStart = false;
  std::string m_startError;
  guint m_portalResponse = 0;
  guint64 m_fixAgeSec = 0;
  std::optional<std::pair<guint64, guint64>> m_fixTime;
  double m_fixAccuracy = kAccuracy;
  double m_fixAltitude = -G_MAXDOUBLE;
  std::string m_locationPath = "/";
  std::string m_portalSession;
  std::string m_desktopId;
};
}  // namespace location::test
