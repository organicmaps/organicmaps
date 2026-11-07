#include "testing/testing.hpp"

#include "platform/location_service/fake_geoclue_services.hpp"
#include "platform/location_service/geoclue_location_service.hpp"

#include <functional>
#include <utility>

namespace geoclue_location_service_tests
{
using namespace location;
using namespace location::test;
using namespace std::chrono_literals;

char constexpr kDesktopId[] = "app.organicmaps.desktop";
char constexpr kAccessDenied[] = "org.freedesktop.DBus.Error.AccessDenied";

struct Observer final : LocationObserver
{
  void OnLocationUpdated(GpsInfo const & info) override
  {
    ++m_locations;
    m_lastInfo = info;
    if (m_onLocation)
      m_onLocation();
  }

  void OnLocationError(TLocationError error) override
  {
    ++m_errors;
    m_lastError = error;
    if (m_onError)
      m_onError();
  }

  int m_locations = 0;
  int m_errors = 0;
  GpsInfo m_lastInfo;
  TLocationError m_lastError = ENoError;
  std::function<void()> m_onLocation;
  std::function<void()> m_onError;
};

void TestFix(FakeGeoClueServices & services)
{
  services.Reset();
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId);
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 1; }), (services.Calls()));
  TEST_EQUAL(services.DesktopId(), kDesktopId, ());
  services.PublishFix();
  TEST(RunUntil([&]() { return observer.m_locations == 1; }), (services.Calls()));
  auto const & info = observer.m_lastInfo;
  TEST_EQUAL(info.m_source, EGeoClue2, ());
  TEST_EQUAL(info.m_latitude, kLatitude, ());
  TEST_EQUAL(info.m_longitude, kLongitude, ());
  TEST_EQUAL(info.m_horizontalAccuracy, kAccuracy, ());
  TEST(!info.HasAltitude(), ());
  TEST(!info.HasSpeed(), ());
  TEST(!info.HasBearing(), ());
  TEST_ALMOST_EQUAL_ABS(info.m_timestamp, g_get_real_time() / 1e6, 5.0, ());
  service.Stop();
  TEST(RunUntil([&]() { return services.Count("DeleteClient") == 1; }), (services.Calls()));
  TEST_EQUAL(services.Count("Stop"), 1, ());
  TEST_EQUAL(observer.m_errors, 0, ());
}

template <typename Publish>
void TestAltitudeWithUnknownAccuracy(FakeGeoClueServices & services, Publish && publish)
{
  double constexpr kAltitude = 34.5;
  services.Reset();
  services.SetFixAccuracy(-1);
  services.SetFixAltitude(kAltitude);
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId);
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 1; }), (services.Calls()));
  publish();
  TEST(RunUntil([&]() { return observer.m_locations == 1; }), (services.Calls()));
  auto const & info = observer.m_lastInfo;
  TEST_EQUAL(info.m_horizontalAccuracy, 100, ());
  TEST(info.HasAltitude(), ());
  TEST_EQUAL(info.m_altitude, kAltitude, ());
  TEST_EQUAL(info.m_verticalAccuracy, 100, ());
  service.Stop();
  RunFor(100);
}

void TestRestartIgnoresOldRequests(FakeGeoClueServices & services)
{
  services.Reset();
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId);
  services.HoldClientCreation(true);
  service.Start();
  TEST(RunUntil([&]() { return services.HeldCount() == 1; }), (services.Calls()));
  service.Stop();
  service.Start();
  TEST(RunUntil([&]() { return services.HeldCount() == 2; }), (services.Calls()));

  // The replies to the first requests come after the second Start().
  services.Answer();
  TEST(RunUntil([&]() { return services.Count("Start") == 1; }), (services.Calls()));
  services.PublishFix();
  TEST(RunUntil([&]() { return observer.m_locations > 0; }), ());
  RunFor(100);
  TEST_EQUAL(observer.m_locations, 1, ());
  TEST_EQUAL(observer.m_errors, 0, ());
  service.Stop();
  RunFor(100);
}

enum class PendingStage
{
  ClientCreation,
  ClientStart,
  ClientProperties,
  Location
};

void TestDestroyWithPendingRequest(FakeGeoClueServices & services, PendingStage stage)
{
  services.Reset();
  services.HoldClientCreation(stage == PendingStage::ClientCreation);
  services.HoldStart(stage == PendingStage::ClientStart);
  services.HoldClientProperties(stage == PendingStage::ClientProperties);
  Observer observer;
  {
    GeoClueLocationService service(observer, kDesktopId);
    service.Start();
    if (stage == PendingStage::Location)
    {
      TEST(RunUntil([&]() { return services.Count("Start") == 1; }), (services.Calls()));
      services.HoldLocation(true);
      services.PublishFix();
    }
    TEST(RunUntil([&]() { return services.HeldCount() > 0; }), (services.Calls()));
  }
  services.Answer();
  services.PublishFix();
  RunFor(200);
  TEST_EQUAL(observer.m_locations + observer.m_errors, 0, ());
  int const clientCalls = stage == PendingStage::ClientCreation ? 0 : 1;
  TEST_EQUAL(services.Count("Stop"), clientCalls, (services.Calls()));
  TEST_EQUAL(services.Count("DeleteClient"), clientCalls, (services.Calls()));
}

void TestStopBeforeFirstFix(FakeGeoClueServices & services)
{
  services.Reset();
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId);
  service.Start();
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 1; }), (services.Calls()));
  service.Stop();
  service.Stop();
  services.PublishFix();
  RunFor(200);
  TEST(!service.IsStarted(), ());
  TEST_EQUAL(services.Count("CreateClient") + services.Count("GetClient"), 1, (services.Calls()));
  TEST_EQUAL(services.Count("Stop"), 1, ());
  TEST_EQUAL(observer.m_locations + observer.m_errors, 0, ());
}

void TestClientDeactivationAndRetry(FakeGeoClueServices & services)
{
  services.Reset();
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId);
  observer.m_onError = [&]() { TEST(!service.IsStarted(), ("Stop the client before notifying the observer")); };
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 1; }), (services.Calls()));
  services.PublishFix();
  TEST(RunUntil([&]() { return observer.m_locations == 1; }), ());
  services.SetActive(false);
  TEST(RunUntil([&]() { return observer.m_errors == 1 && services.Count("Stop") == 1; }), (services.Calls()));
  TEST_EQUAL(observer.m_lastError, EGPSIsOff, ());
  services.PublishFix();
  RunFor(100);
  TEST_EQUAL(observer.m_locations, 1, ());
  TEST_EQUAL(observer.m_errors, 1, ());

  services.SetActive(true);
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 2 && observer.m_locations == 2; }), (services.Calls()));
  TEST(service.IsStarted(), ());
  TEST_EQUAL(observer.m_errors, 1, ());
  service.Stop();
  services.SetActive(false);
  RunFor(100);
  TEST_EQUAL(observer.m_errors, 1, ());
}

void TestDeactivationBeforeFirstFix(FakeGeoClueServices & services)
{
  services.Reset();
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId);
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 1; }), (services.Calls()));
  services.SetActive(false);
  TEST(RunUntil([&]() { return observer.m_errors == 1; }), (services.Calls()));
  TEST(!service.IsStarted(), ());
  TEST_EQUAL(observer.m_lastError, EGPSIsOff, ());
  services.PublishFix();
  RunFor(100);
  TEST_EQUAL(observer.m_locations, 0, ());
  TEST_EQUAL(observer.m_errors, 1, ());
  service.Start();
  TEST(RunUntil([&]() { return observer.m_locations == 1; }), (services.Calls()));
  TEST(service.IsStarted(), ());
  service.Stop();
  RunFor(100);
}

void TestInactiveDuringStart(FakeGeoClueServices & services)
{
  services.Reset();
  services.HoldStart(true);
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId);
  service.Start();
  TEST(RunUntil([&]() { return services.HeldCount() == 1; }), (services.Calls()));
  services.SetActive(false);
  RunFor(100);
  TEST(service.IsStarted(), ());
  TEST_EQUAL(observer.m_errors, 0, ());
  // A successful Start whose resulting Active is false must be detected without another notify.
  services.DeactivateOnStart(true);
  services.Answer();
  TEST(RunUntil([&]() { return observer.m_errors == 1; }), (services.Calls()));
  TEST_EQUAL(observer.m_lastError, EGPSIsOff, ());
  TEST(!service.IsStarted(), ());
  RunFor(100);
}

void TestStartBeforePropertyNotifications(FakeGeoClueServices & services)
{
  services.Reset();
  services.DelayStartNotifications(true);
  services.PublishLocationDuringStart(true);
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId);
  service.Start();
  // Active and Location are current on the server, but the proxy still caches false and "/".
  TEST(RunUntil([&]() { return observer.m_locations == 1; }), (services.Calls()));
  TEST(service.IsStarted(), ());
  TEST_EQUAL(observer.m_errors, 0, ());
  services.SetActive(true);
  RunFor(100);
  TEST_EQUAL(observer.m_locations, 1, ());
  service.Stop();
  RunFor(100);
}

void TestDeactivationWithPendingClientProperties(FakeGeoClueServices & services)
{
  services.Reset();
  services.HoldClientProperties(true);
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId);
  service.Start();
  TEST(RunUntil([&]() { return services.HeldCount() == 1; }), (services.Calls()));
  services.SetActive(false);
  TEST(RunUntil([&]() { return observer.m_errors == 1; }), ());
  TEST(!service.IsStarted(), ());
  TEST_EQUAL(observer.m_lastError, EGPSIsOff, ());
  services.Answer();
  services.PublishFix();
  RunFor(100);
  TEST_EQUAL(observer.m_locations, 0, ());
  TEST_EQUAL(observer.m_errors, 1, ());
}

void TestLocationDuringClientProperties(FakeGeoClueServices & services)
{
  services.Reset();
  services.HoldClientProperties(true);
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId);
  service.Start();
  TEST(RunUntil([&]() { return services.HeldCount() == 1; }), (services.Calls()));
  services.PublishFix();
  TEST(RunUntil([&]() { return observer.m_locations == 1; }), ());
  services.Answer();
  RunFor(100);
  TEST_EQUAL(observer.m_locations, 1, ("The initial snapshot must not replay a newer LocationUpdated"));
  TEST_EQUAL(observer.m_errors, 0, ());
  service.Stop();
  RunFor(100);
}

void TestStartFailureAndRetry(FakeGeoClueServices & services)
{
  for (auto const & [dbusError, expected] :
       {std::pair{kAccessDenied, EDenied}, std::pair{"org.freedesktop.DBus.Error.Failed", EUnknown}})
  {
    services.Reset();
    services.FailStart(dbusError);
    Observer observer;
    GeoClueLocationService service(observer, kDesktopId);
    service.Start();
    TEST(RunUntil([&]() { return observer.m_errors == 1; }), (services.Calls()));
    TEST_EQUAL(observer.m_lastError, expected, (dbusError));
    TEST(!service.IsStarted(), ());
    services.FailStart({});
    service.Start();
    TEST(RunUntil([&]() { return services.Count("Start") == 2; }), (services.Calls()));
    services.PublishFix();
    TEST(RunUntil([&]() { return observer.m_locations == 1; }), ());
    service.Stop();
    RunFor(100);
  }
}

void TestObserverRestartsOnError(FakeGeoClueServices & services)
{
  services.Reset();
  services.FailStart(kAccessDenied);
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId);
  observer.m_onError = [&]()
  {
    services.FailStart({});
    service.Start();
  };
  service.Start();
  TEST(RunUntil([&]() { return observer.m_errors == 1 && services.Count("Start") == 2; }), (services.Calls()));
  TEST(service.IsStarted(), ());
  services.PublishFix();
  TEST(RunUntil([&]() { return observer.m_locations == 1; }), ());
  TEST_EQUAL(observer.m_errors, 1, ());
  service.Stop();
  RunFor(100);
}

void TestObserverStopsOnFix(FakeGeoClueServices & services)
{
  services.Reset();
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId);
  observer.m_onLocation = [&]() { service.Stop(); };
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 1; }), (services.Calls()));
  services.PublishFix();
  TEST(RunUntil([&]() { return services.Count("Stop") == 1; }), (services.Calls()));
  TEST(!service.IsStarted(), ());
  observer.m_onLocation = {};
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 2; }), (services.Calls()));
  services.PublishFix();
  TEST(RunUntil([&]() { return observer.m_locations == 2; }), ());
  TEST_EQUAL(observer.m_errors, 0, ());
  service.Stop();
  RunFor(100);
}

void TestNoService(FakeGeoClueServices & services)
{
  services.Reset();
  services.SetGeoClueAvailable(false);
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId);
  service.Start();
  TEST(RunUntil([&]() { return observer.m_errors == 1; }), (services.Calls()));
  TEST_EQUAL(observer.m_lastError, ENotSupported, ());
  TEST(!service.IsStarted(), ());
  services.SetGeoClueAvailable(true);
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 1; }), (services.Calls()));
  services.PublishFix();
  TEST(RunUntil([&]() { return observer.m_locations == 1; }), ());
  service.Stop();
  RunFor(100);
}

void TestSupersededLocationRequest(FakeGeoClueServices & services)
{
  services.Reset();
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId);
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 1; }), (services.Calls()));
  services.HoldLocation(true);
  services.PublishFix();
  TEST(RunUntil([&]() { return services.HeldCount() > 0; }), (services.Calls()));
  services.HoldLocation(false);
  services.PublishFix(kSecondLocationPath);
  TEST(RunUntil([&]() { return observer.m_locations == 1; }), (services.Calls()));
  // The first proxy request completes after the newer position has already been delivered.
  services.Answer();
  RunFor(100);
  TEST_EQUAL(observer.m_locations, 1, ());
  TEST_EQUAL(observer.m_errors, 0, ());
  service.Stop();
  RunFor(100);
}

void TestUnavailableLocationObject(FakeGeoClueServices & services)
{
  services.Reset();
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId);
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 1; }), (services.Calls()));
  // A proxy for an object that is not exported has default properties and no timestamp.
  services.PublishFix("/org/freedesktop/GeoClue2/Client/1/Location/Missing");
  RunFor(100);
  TEST_EQUAL(observer.m_locations, 0, ());
  services.PublishFix();
  TEST(RunUntil([&]() { return observer.m_locations == 1; }), (services.Calls()));
  RunFor(100);
  TEST_EQUAL(observer.m_locations, 1, ());
  TEST_EQUAL(observer.m_errors, 0, ());
  service.Stop();
  RunFor(100);
}

void TestOrderKeptAcrossRestart(FakeGeoClueServices & services)
{
  services.Reset();
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId);
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 1; }), (services.Calls()));
  services.PublishFix();
  TEST(RunUntil([&]() { return observer.m_locations == 1; }), ());
  service.Stop();

  // A restart must not accept a fix older than the one already shown.
  services.SetFixAge(10);
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 2; }), (services.Calls()));
  services.PublishFix();
  RunFor(200);
  TEST_EQUAL(observer.m_locations, 1, ());
  services.SetFixAge(0);
  services.PublishFix();
  TEST(RunUntil([&]() { return observer.m_locations == 2; }), ());
  TEST_EQUAL(observer.m_errors, 0, ());
  service.Stop();
}

void TestOrderWithinOneSecond(FakeGeoClueServices & services)
{
  services.Reset();
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId);
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 1; }), (services.Calls()));
  guint64 const sec = g_get_real_time() / G_USEC_PER_SEC;
  int expected = 0;
  // Microseconds of the provider timestamp order the fixes: equal timestamps pass, older ones don't.
  for (auto const & [usec, accepted] :
       {std::pair{500000, true}, std::pair{100000, false}, std::pair{900000, true}, std::pair{900000, true}})
  {
    services.SetFixTime(sec, usec);
    services.PublishFix();
    if (accepted)
      ++expected;
    RunFor(100);
    TEST_EQUAL(observer.m_locations, expected, (usec));
  }
  service.Stop();
  RunFor(100);
}

void TestFirstFixTimeout(FakeGeoClueServices & services)
{
  services.Reset();
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId, 300ms);
  observer.m_onError = [&]() { TEST(!service.IsStarted(), ("Stop the client before notifying the observer")); };
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 1; }), (services.Calls()));
  // An outdated fix must not end the acquisition.
  services.SetFixAge(3600);
  services.PublishFix();
  TEST(RunUntil([&]() { return observer.m_errors == 1; }), (services.Calls()));
  TEST_EQUAL(observer.m_lastError, ETimeout, ());
  TEST_EQUAL(observer.m_locations, 0, ());
  TEST(RunUntil([&]() { return services.Count("Stop") == 1; }), (services.Calls()));
  services.SetFixAge(0);
  services.PublishFix();
  RunFor(100);
  TEST_EQUAL(observer.m_locations, 0, ());

  // A fix ends the acquisition; a repeated Start must not rearm the deadline.
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 2; }), (services.Calls()));
  services.PublishFix();
  TEST(RunUntil([&]() { return observer.m_locations == 1; }), ());
  service.Start();
  RunFor(600);
  TEST_EQUAL(observer.m_errors, 1, ());
  TEST(service.IsStarted(), ());
  service.Stop();

  // Stop() cancels the deadline.
  service.Start();
  service.Stop();
  RunFor(600);
  TEST_EQUAL(observer.m_errors, 1, ());
}

void TestPortalLifecycle(FakeGeoClueServices & services)
{
  services.Reset();
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId);
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 1; }), (services.Calls()));
  services.PublishPortalFix();
  TEST(RunUntil([&]() { return observer.m_locations == 1; }), (services.Calls()));
  TEST_EQUAL(observer.m_lastInfo.m_latitude, kLatitude, ());
  TEST_EQUAL(observer.m_lastInfo.m_longitude, kLongitude, ());
  services.PublishPortalFix();
  TEST(RunUntil([&]() { return observer.m_locations == 2; }), ());
  TEST_EQUAL(services.Count("CreateClient"), 0, ());
  TEST_EQUAL(services.Count("GetClient"), 0, ());
  service.Stop();
  TEST(RunUntil([&]() { return services.Count("Close") == 1; }), (services.Calls()));
  services.PublishPortalFix();
  RunFor(100);
  TEST_EQUAL(observer.m_locations, 2, ());
  TEST_EQUAL(observer.m_errors, 0, ());

  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 2; }), (services.Calls()));
  service.Stop();
  TEST(RunUntil([&]() { return services.Count("Close") == 2; }), (services.Calls()));
  // Cancellation must close the session without waiting for a first fix.
  services.PublishPortalFix();
  RunFor(100);
  TEST_EQUAL(observer.m_locations, 2, ());
  TEST_EQUAL(observer.m_errors, 0, ());
}

void TestPortalStopOnFirstFix(FakeGeoClueServices & services)
{
  services.Reset();
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId);
  observer.m_onLocation = [&]() { service.Stop(); };
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 1; }), (services.Calls()));
  services.PublishPortalFix();
  TEST(RunUntil([&]() { return services.Count("Close") == 1; }), (services.Calls()));
  TEST(!service.IsStarted(), ());
  services.PublishPortalFix();
  RunFor(100);
  TEST_EQUAL(observer.m_locations, 1, ());
  TEST_EQUAL(observer.m_errors, 0, ());
}

void TestPortalDenialAndRetry(FakeGeoClueServices & services)
{
  services.Reset();
  services.SetPortalResponse(1);
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId);
  service.Start();
  TEST(RunUntil([&]() { return observer.m_errors == 1; }), (services.Calls()));
  // GClueSimple doesn't tell a denial from other portal failures.
  TEST_EQUAL(observer.m_lastError, EUnknown, ());
  TEST(!service.IsStarted(), ());
  TEST(RunUntil([&]() { return services.Count("Close") == 1; }), (services.Calls()));
  services.SetPortalResponse(0);
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 2; }), (services.Calls()));
  services.PublishPortalFix();
  TEST(RunUntil([&]() { return observer.m_locations == 1; }), (services.Calls()));
  TEST_EQUAL(observer.m_errors, 1, ());
  service.Stop();
  TEST(RunUntil([&]() { return services.Count("Close") == 2; }), (services.Calls()));
}

void TestPortalStopDuringStart(FakeGeoClueServices & services)
{
  services.Reset();
  services.HoldStart(true);
  Observer observer;
  GeoClueLocationService service(observer, kDesktopId);
  service.Start();
  TEST(RunUntil([&]() { return services.HeldCount() == 1; }), (services.Calls()));
  service.Stop();
  services.Answer();
  TEST(RunUntil([&]() { return services.Count("Close") == 1; }), (services.Calls()));
  services.PublishPortalFix();
  RunFor(100);
  TEST_EQUAL(observer.m_locations + observer.m_errors, 0, ());
}

// Every async request may complete after Stop(), the next Start() or the destruction of the service.
// One bus for all cases: the system bus singleton is created once.
UNIT_TEST(GeoClueLocationService_AsyncLifecycle)
{
  FakeGeoClueServices services;
  if (g_strcmp0(g_getenv("GTK_USE_PORTAL"), "1") == 0)
  {
    TestPortalLifecycle(services);
    TestAltitudeWithUnknownAccuracy(services, [&]() { services.PublishPortalFix(); });
    TestPortalStopOnFirstFix(services);
    TestPortalDenialAndRetry(services);
    TestPortalStopDuringStart(services);
    // Cancelled calls complete from the main loop.
    RunFor(100);
    return;
  }
  TestFix(services);
  TestAltitudeWithUnknownAccuracy(services, [&]() { services.PublishFix(); });
  TestRestartIgnoresOldRequests(services);
  for (auto const stage : {PendingStage::ClientCreation, PendingStage::ClientStart, PendingStage::ClientProperties,
                           PendingStage::Location})
    TestDestroyWithPendingRequest(services, stage);
  TestStopBeforeFirstFix(services);
  TestClientDeactivationAndRetry(services);
  TestDeactivationBeforeFirstFix(services);
  TestInactiveDuringStart(services);
  TestStartBeforePropertyNotifications(services);
  TestDeactivationWithPendingClientProperties(services);
  TestLocationDuringClientProperties(services);
  TestStartFailureAndRetry(services);
  TestObserverRestartsOnError(services);
  TestObserverStopsOnFix(services);
  TestNoService(services);
  TestSupersededLocationRequest(services);
  TestUnavailableLocationObject(services);
  TestOrderKeptAcrossRestart(services);
  TestOrderWithinOneSecond(services);
  TestFirstFixTimeout(services);
  // Cancelled calls complete from the main loop.
  RunFor(100);
}
}  // namespace geoclue_location_service_tests
