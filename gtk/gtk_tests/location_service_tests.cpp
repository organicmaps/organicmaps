#include "testing/testing.hpp"

#include "gtk/location.hpp"

#include "base/assert.hpp"

#include <gio/gio.h>

#include <algorithm>
#include <string>
#include <vector>

namespace location_service_tests
{
char constexpr kXml[] = R"xml(
<node>
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
</node>)xml";

char constexpr kClientPath[] = "/org/freedesktop/GeoClue2/Client/1";
char constexpr kLocationPath[] = "/org/freedesktop/GeoClue2/Client/1/Location/1";
char constexpr kCompassPath[] = "/net/hadess/SensorProxy/Compass";

// Runs the default main context, which serves both the services and the tested code.
template <typename Pred>
bool RunUntil(Pred && pred)
{
  gint64 const deadline = g_get_monotonic_time() + 5 * G_USEC_PER_SEC;
  guint const tick = g_timeout_add(10, [](gpointer) -> gboolean { return G_SOURCE_CONTINUE; }, nullptr);
  while (!pred() && g_get_monotonic_time() < deadline)
    g_main_context_iteration(nullptr, TRUE);
  g_source_remove(tick);
  return pred();
}

void RunFor(int ms)
{
  gint64 const end = g_get_monotonic_time() + ms * 1000;
  RunUntil([end]() { return g_get_monotonic_time() >= end; });
}

// GeoClue and iio-sensor-proxy on a private system bus. Calls that create a GeoClue client or claim
// the compass may be held to complete after the service has been stopped.
class FakeServices
{
public:
  FakeServices()
  {
    m_bus = g_test_dbus_new(G_TEST_DBUS_NONE);
    g_test_dbus_up(m_bus);
    char const * address = g_test_dbus_get_bus_address(m_bus);
    CHECK(address, ("dbus-daemon is required"));
    g_setenv("DBUS_SYSTEM_BUS_ADDRESS", address, TRUE);
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
    Register(kLocationPath, "org.freedesktop.GeoClue2.Location");
    Register(kCompassPath, "net.hadess.SensorProxy.Compass");
    for (char const * name : {"org.freedesktop.GeoClue2", "net.hadess.SensorProxy"})
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
    g_test_dbus_down(m_bus);
    g_object_unref(m_bus);
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
    for (auto * invocation : m_held)
      Reply(invocation);
    m_held.clear();
  }

  // Forgets the calls and the published values of the previous case.
  void Reset()
  {
    m_calls.clear();
    m_hasLocation = false;
    m_heading = -1;
  }

  size_t HeldCount() const { return m_held.size(); }
  std::vector<std::string> const & Calls() const { return m_calls; }
  int Count(std::string const & method) const { return static_cast<int>(std::ranges::count(m_calls, method)); }
  // The position of the first call, or the number of calls.
  int Index(std::string const & method) const
  {
    return static_cast<int>(std::ranges::find(m_calls, method) - m_calls.begin());
  }

  void PublishFix()
  {
    m_hasLocation = true;
    g_dbus_connection_emit_signal(m_conn, nullptr, kClientPath, "org.freedesktop.GeoClue2.Client", "LocationUpdated",
                                  g_variant_new("(oo)", "/", kLocationPath), nullptr);
  }

  void SetHeading(double degrees)
  {
    m_heading = degrees;
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

  static void Reply(GDBusMethodInvocation * invocation)
  {
    std::string const method = g_dbus_method_invocation_get_method_name(invocation);
    if (method == "GetClient" || method == "CreateClient")
      g_dbus_method_invocation_return_value(invocation, g_variant_new("(o)", kClientPath));
    else
      g_dbus_method_invocation_return_value(invocation, nullptr);
  }

  static void OnMethodCall(GDBusConnection *, char const *, char const *, char const *, char const * method, GVariant *,
                           GDBusMethodInvocation * invocation, gpointer data)
  {
    auto * self = static_cast<FakeServices *>(data);
    std::string const name = method;
    self->m_calls.push_back(name);
    bool const createsClient = name == "GetClient" || name == "CreateClient";
    if ((createsClient && self->m_holdGeoClue) || (name == "ClaimCompass" && self->m_holdCompass))
      self->m_held.push_back(invocation);
    else
      Reply(invocation);
  }

  static GVariant * OnGetProperty(GDBusConnection *, char const *, char const *, char const *, char const * prop,
                                  GError **, gpointer data)
  {
    auto const * self = static_cast<FakeServices *>(data);
    std::string const name = prop;
    if (name == "InUse" || name == "Active" || name == "HasCompass")
      return g_variant_new_boolean(TRUE);
    if (name == "AvailableAccuracyLevel")
      return g_variant_new_uint32(8);
    if (name == "DistanceThreshold" || name == "TimeThreshold" || name == "RequestedAccuracyLevel")
      return g_variant_new_uint32(0);
    if (name == "DesktopId" || name == "Description")
      return g_variant_new_string("");
    if (name == "Location")
      return g_variant_new_object_path(self->m_hasLocation ? kLocationPath : "/");
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
  GDBusConnection * m_conn = nullptr;
  GDBusNodeInfo * m_node = nullptr;
  std::vector<guint> m_registrations;
  std::vector<GDBusMethodInvocation *> m_held;
  std::vector<std::string> m_calls;
  bool m_holdGeoClue = false;
  bool m_holdCompass = false;
  bool m_hasLocation = false;
  double m_heading = -1;
};

struct Counters
{
  int m_locations = 0;
  int m_errors = 0;
  int m_headings = 0;
};

void Watch(gtk::LocationService & service, Counters & counters)
{
  service.m_onLocation = [&counters](location::GpsInfo const &) { ++counters.m_locations; };
  service.m_onError = [&counters](location::TLocationError) { ++counters.m_errors; };
  service.m_onCompass = [&counters](double) { ++counters.m_headings; };
}

void TestRestartIgnoresOldRequests(FakeServices & services)
{
  services.Reset();
  Counters counters;
  gtk::LocationService service;
  Watch(service, counters);
  services.Hold(true /* geoClue */, true /* compass */);
  service.Start();
  TEST(RunUntil([&]() { return services.HeldCount() == 2; }), (services.Calls()));
  service.Stop();
  service.Start();
  TEST(RunUntil([&]() { return services.HeldCount() == 4; }), (services.Calls()));

  // The replies to the first requests come after the second Start().
  services.Answer();
  TEST(RunUntil([&]() { return services.Count("Start") == 1; }), (services.Calls()));
  services.PublishFix();
  TEST(RunUntil([&]() { return counters.m_locations > 0; }), ());
  services.SetHeading(90);
  TEST(RunUntil([&]() { return counters.m_headings > 0; }), ());
  RunFor(100);
  TEST_EQUAL(counters.m_locations, 1, ());
  TEST_EQUAL(counters.m_headings, 1, ());
  TEST_EQUAL(counters.m_errors, 0, ());
  TEST_EQUAL(services.Count("ClaimCompass"), 2, ());
  TEST_EQUAL(services.Count("ReleaseCompass"), 1, ());

  service.Stop();
  TEST(RunUntil([&]() { return services.Count("ReleaseCompass") == 2; }), (services.Calls()));
  RunFor(100);
}

void TestDestroyWithPendingRequests(FakeServices & services)
{
  services.Reset();
  Counters counters;
  {
    gtk::LocationService service;
    Watch(service, counters);
    services.Hold(true /* geoClue */, true /* compass */);
    service.Start();
    TEST(RunUntil([&]() { return services.HeldCount() == 2; }), (services.Calls()));
  }
  services.Answer();
  services.PublishFix();
  services.SetHeading(45);
  RunFor(200);
  TEST_EQUAL(counters.m_locations + counters.m_errors + counters.m_headings, 0, ());
  // Released although the claim was not answered yet.
  TEST_LESS(services.Index("ClaimCompass"), services.Index("ReleaseCompass"), (services.Calls()));
}

void TestStopDuringCompassClaim(FakeServices & services)
{
  services.Reset();
  Counters counters;
  gtk::LocationService service;
  Watch(service, counters);
  services.Hold(false /* geoClue */, true /* compass */);
  service.Start();
  TEST(RunUntil([&]() { return services.HeldCount() == 1; }), (services.Calls()));
  service.Stop();
  TEST(RunUntil([&]() { return services.Count("ReleaseCompass") == 1; }), (services.Calls()));
  services.Answer();
  services.SetHeading(180);
  RunFor(200);
  TEST_LESS(services.Index("ClaimCompass"), services.Index("ReleaseCompass"), (services.Calls()));
  TEST_EQUAL(counters.m_headings, 0, ());
  TEST_EQUAL(counters.m_locations, 0, ());
}

// Every async request of LocationService may complete after Stop(), the next Start() or the
// destruction of the service. One bus for all cases: the system bus singleton is created once.
UNIT_TEST(LocationService_AsyncLifecycle)
{
  FakeServices services;
  TestRestartIgnoresOldRequests(services);
  TestDestroyWithPendingRequests(services);
  TestStopDuringCompassClaim(services);
  // Cancelled calls complete from the main loop.
  RunFor(100);
}
}  // namespace location_service_tests
