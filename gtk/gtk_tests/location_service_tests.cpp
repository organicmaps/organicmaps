#include "testing/testing.hpp"

#include "gtk/gtk_tests/fake_location_services.hpp"

#include <cmath>
#include <limits>

namespace location_service_tests
{
using namespace gtk_test_support;

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

void TestStopBeforeFirstFix(FakeServices & services)
{
  services.Reset();
  Counters counters;
  gtk::LocationService service;
  Watch(service, counters);
  service.Start();
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 1 && services.Count("ClaimCompass") == 1; }),
       (services.Calls()));
  service.Stop();
  service.Stop();
  services.PublishFix();
  services.SetHeading(90);
  RunFor(200);
  TEST(!service.IsStarted(), ());
  TEST_EQUAL(counters.m_locations + counters.m_errors + counters.m_headings, 0, ());
  TEST_EQUAL(services.Count("ReleaseCompass"), 1, ());
}

void TestClientDeactivationAndRetry(FakeServices & services)
{
  services.Reset();
  services.SetHeading(90, false /* notify */);
  Counters counters;
  gtk::LocationService service;
  Watch(service, counters);
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 1 && counters.m_headings == 1; }), (services.Calls()));
  services.PublishFix();
  TEST(RunUntil([&]() { return counters.m_locations == 1; }), ());
  services.SetActive(false);
  TEST(RunUntil([&]() { return counters.m_errors == 1 && services.Count("ReleaseCompass") == 1; }), ());
  TEST(!service.IsStarted(), ());
  TEST_EQUAL(counters.m_lastError, location::EGPSIsOff, ());
  services.PublishFix();
  services.SetHeading(180);
  RunFor(100);
  TEST_EQUAL(counters.m_locations, 1, ());
  TEST_EQUAL(counters.m_headings, 1, ());
  TEST_EQUAL(counters.m_errors, 1, ());

  services.SetActive(true);
  service.Start();
  TEST(
      RunUntil([&]() { return services.Count("Start") == 2 && counters.m_locations == 2 && counters.m_headings == 2; }),
      (services.Calls()));
  TEST(service.IsStarted(), ());
  TEST_EQUAL(counters.m_errors, 1, ());
  service.Stop();
  services.SetActive(false);
  RunFor(100);
  TEST_EQUAL(counters.m_errors, 1, ());
}

void TestInitialCompassHeading(FakeServices & services)
{
  services.Reset();
  services.SetHeading(90, false /* notify */);
  Counters counters;
  gtk::LocationService service;
  Watch(service, counters);
  service.Start();
  TEST(RunUntil([&]() { return counters.m_headings == 1; }), (services.Calls()));
  TEST_ALMOST_EQUAL_ABS(counters.m_lastHeading, M_PI / 2, 1e-12, ());
  services.SetHasCompass(false);
  RunFor(50);
  services.SetHasCompass(true);
  RunFor(50);
  TEST_EQUAL(counters.m_headings, 1, ("Unrelated properties must not replay the cached heading"));
  services.SetHeading(0);
  TEST(RunUntil([&]() { return counters.m_headings == 2; }), ());
  TEST_EQUAL(counters.m_lastHeading, 0.0, ());
  services.SetHeading(-1);
  RunFor(50);
  services.SetHeading(std::numeric_limits<double>::quiet_NaN());
  RunFor(50);
  TEST_EQUAL(counters.m_headings, 2, ());
  service.Stop();
  RunFor(100);
}

void TestPermissionFailureAndRetry(FakeServices & services)
{
  services.Reset();
  services.DenyLocation(true);
  Counters counters;
  gtk::LocationService service;
  Watch(service, counters);
  service.Start();
  TEST(RunUntil([&]() { return counters.m_errors == 1; }), (services.Calls()));
  TEST_EQUAL(counters.m_lastError, location::EDenied, ());
  TEST(!service.IsStarted(), ());
  services.DenyLocation(false);
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 2; }), (services.Calls()));
  services.PublishFix();
  TEST(RunUntil([&]() { return counters.m_locations == 1; }), ());
  service.Stop();
  RunFor(100);
}

void TestCompassFailureKeepsLocation(FakeServices & services)
{
  services.Reset();
  services.FailCompassClaim(true);
  services.SetHeading(90, false /* notify */);
  Counters counters;
  gtk::LocationService service;
  Watch(service, counters);
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 1 && services.Count("ClaimCompass") == 1; }),
       (services.Calls()));
  services.PublishFix();
  TEST(RunUntil([&]() { return counters.m_locations == 1; }), ());
  RunFor(100);
  TEST(service.IsStarted(), ());
  TEST_EQUAL(counters.m_headings, 0, ());
  TEST_EQUAL(counters.m_errors, 0, ());
  service.Stop();
  RunFor(100);
}

void TestDeactivationBeforeFirstFix(FakeServices & services)
{
  services.Reset();
  Counters counters;
  gtk::LocationService service;
  Watch(service, counters);
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 1 && services.Count("ClaimCompass") == 1; }),
       (services.Calls()));
  services.SetActive(false);
  TEST(RunUntil([&]() { return counters.m_errors == 1 && services.Count("ReleaseCompass") == 1; }), (services.Calls()));
  TEST(!service.IsStarted(), ());
  TEST_EQUAL(counters.m_lastError, location::EGPSIsOff, ());
  services.PublishFix();
  services.SetHeading(90);
  RunFor(100);
  TEST_EQUAL(counters.m_locations + counters.m_headings, 0, ());
  TEST_EQUAL(counters.m_errors, 1, ());
  service.Start();
  TEST(RunUntil([&]() { return counters.m_locations == 1 && counters.m_headings == 1; }), (services.Calls()));
  TEST(service.IsStarted(), ());
  service.Stop();
  RunFor(100);
}

void TestInactiveDuringStart(FakeServices & services)
{
  services.Reset();
  services.HoldStart(true);
  Counters counters;
  gtk::LocationService service;
  Watch(service, counters);
  service.Start();
  TEST(RunUntil([&]() { return services.HeldCount() == 1; }), (services.Calls()));
  services.SetActive(false);
  RunFor(100);
  TEST(service.IsStarted(), ());
  TEST_EQUAL(counters.m_errors, 0, ());
  // A successful Start whose resulting Active is false must be detected without another notify.
  services.DeactivateOnStart(true);
  services.Answer();
  TEST(RunUntil([&]() { return counters.m_errors == 1; }), (services.Calls()));
  TEST_EQUAL(counters.m_lastError, location::EGPSIsOff, ());
  TEST(!service.IsStarted(), ());
  RunFor(100);
}

void TestCompassTimeoutKeepsLocation(FakeServices & services)
{
  services.Reset();
  services.Hold(false /* geoClue */, true /* compass */);
  services.SetHeading(90, false /* notify */);
  Counters counters;
  gtk::LocationService service;
  Watch(service, counters);
  service.Start();
  TEST(RunUntil([&]() { return services.HeldCount() == 1 && services.Count("Start") == 1; }), (services.Calls()));
  services.PublishFix();
  TEST(RunUntil([&]() { return counters.m_locations == 1; }), ());
  // Exercise the real GIO default timeout, after the server has already accepted the claim.
  TEST(RunUntil([&]() { return services.Count("ReleaseCompass") == 1; }, 30000), (services.Calls()));
  TEST(service.IsStarted(), ());
  TEST_EQUAL(counters.m_errors, 0, ());
  services.Answer();
  services.SetHeading(180);
  services.PublishFix();
  TEST(RunUntil([&]() { return counters.m_locations == 2; }), ());
  RunFor(100);
  TEST_EQUAL(counters.m_headings, 0, ());
  service.Stop();
  RunFor(100);
  TEST_EQUAL(services.Count("ReleaseCompass"), 1, ());
}

void TestCompassHotplugAndRestart(FakeServices & services)
{
  services.Reset();
  services.SetHasCompass(false);
  Counters counters;
  gtk::LocationService service;
  Watch(service, counters);
  service.Start();
  TEST(RunUntil([&]() { return services.Count("ClaimCompass") == 1 && services.Count("Start") == 1; }),
       (services.Calls()));
  services.SetHasCompass(true);
  services.SetHeading(90);
  TEST(RunUntil([&]() { return counters.m_headings == 1; }), ());
  services.SetCompassServiceAvailable(false);
  RunFor(100);
  services.SetHeading(180, false /* notify */);
  services.SetCompassServiceAvailable(true);
  TEST(RunUntil([&]() { return services.Count("ClaimCompass") == 2 && counters.m_headings == 2; }), (services.Calls()));
  TEST_ALMOST_EQUAL_ABS(counters.m_lastHeading, M_PI, 1e-12, ());
  services.PublishFix();
  TEST(RunUntil([&]() { return counters.m_locations == 1; }), ());
  TEST_EQUAL(counters.m_errors, 0, ());
  TEST_EQUAL(services.Count("Start"), 1, ());
  service.Stop();
  TEST(RunUntil([&]() { return services.Count("ReleaseCompass") == 1; }), (services.Calls()));
  RunFor(100);
}

void TestCompassRestartDuringClaim(FakeServices & services)
{
  services.Reset();
  services.Hold(false /* geoClue */, true /* compass */);
  Counters counters;
  gtk::LocationService service;
  Watch(service, counters);
  service.Start();
  TEST(RunUntil([&]() { return services.HeldCount() == 1; }), (services.Calls()));
  services.SetCompassServiceAvailable(false);
  RunFor(100);
  services.SetHeading(90, false /* notify */);
  services.SetCompassServiceAvailable(true);
  TEST(RunUntil([&]() { return services.HeldCount() == 2; }), (services.Calls()));
  // Both owners answer; only the current claim may deliver the cached heading.
  services.Answer();
  TEST(RunUntil([&]() { return counters.m_headings == 1; }), (services.Calls()));
  RunFor(100);
  TEST_EQUAL(counters.m_headings, 1, ());
  TEST_EQUAL(counters.m_errors, 0, ());
  service.Stop();
  TEST(RunUntil([&]() { return services.Count("ReleaseCompass") == 1; }), (services.Calls()));
  RunFor(100);
}

void TestStopDuringClientStart(FakeServices & services)
{
  services.Reset();
  services.HoldStart(true);
  Counters counters;
  {
    gtk::LocationService service;
    Watch(service, counters);
    service.Start();
    TEST(RunUntil([&]() { return services.HeldCount() == 1; }), (services.Calls()));
  }
  services.Answer();
  services.PublishFix();
  services.SetHeading(90);
  RunFor(100);
  TEST_EQUAL(counters.m_locations + counters.m_headings + counters.m_errors, 0, ());
  TEST_EQUAL(services.Count("Stop"), 1, ());
  TEST_EQUAL(services.Count("DeleteClient"), 1, ());
}

void TestSupersededLocationRequest(FakeServices & services)
{
  services.Reset();
  Counters counters;
  gtk::LocationService service;
  Watch(service, counters);
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 1; }), (services.Calls()));
  services.HoldLocation(true);
  services.PublishFix();
  TEST(RunUntil([&]() { return services.HeldCount() > 0; }), (services.Calls()));
  services.HoldLocation(false);
  services.PublishFix(kSecondLocationPath);
  TEST(RunUntil([&]() { return counters.m_locations == 1; }), (services.Calls()));
  // The first proxy request completes after the newer position has already been delivered.
  services.Answer();
  RunFor(100);
  TEST_EQUAL(counters.m_locations, 1, ());
  TEST_EQUAL(counters.m_errors, 0, ());
  service.Stop();
  RunFor(100);
}

void TestDestroyWithPendingLocation(FakeServices & services)
{
  services.Reset();
  Counters counters;
  {
    gtk::LocationService service;
    Watch(service, counters);
    service.Start();
    TEST(RunUntil([&]() { return services.Count("Start") == 1; }), (services.Calls()));
    services.HoldLocation(true);
    services.PublishFix();
    TEST(RunUntil([&]() { return services.HeldCount() > 0; }), (services.Calls()));
  }
  services.Answer();
  RunFor(100);
  TEST_EQUAL(counters.m_locations + counters.m_headings + counters.m_errors, 0, ());
}

void TestPortalLifecycle(FakeServices & services)
{
  Counters counters;
  gtk::LocationService service;
  Watch(service, counters);
  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 1; }), (services.Calls()));
  services.PublishPortalFix();
  TEST(RunUntil([&]() { return counters.m_locations == 1; }), (services.Calls()));
  services.PublishPortalFix();
  TEST(RunUntil([&]() { return counters.m_locations == 2; }), ());
  TEST_EQUAL(services.Count("CreateClient"), 0, ());
  TEST_EQUAL(services.Count("GetClient"), 0, ());
  service.Stop();
  TEST(RunUntil([&]() { return services.Count("Close") == 1; }), (services.Calls()));
  services.PublishPortalFix();
  RunFor(100);
  TEST_EQUAL(counters.m_locations, 2, ());
  TEST_EQUAL(counters.m_errors, 0, ());

  service.Start();
  TEST(RunUntil([&]() { return services.Count("Start") == 2; }), (services.Calls()));
  service.Stop();
  TEST(RunUntil([&]() { return services.Count("Close") == 2; }), (services.Calls()));
  // Cancellation must close the session without waiting for a first fix.
  services.PublishPortalFix();
  RunFor(100);
  TEST_EQUAL(counters.m_locations, 2, ());
  TEST_EQUAL(counters.m_errors, 0, ());
}

// Every async request of LocationService may complete after Stop(), the next Start() or the
// destruction of the service. One bus for all cases: the system bus singleton is created once.
UNIT_TEST(LocationService_AsyncLifecycle)
{
  FakeServices services;
  if (g_strcmp0(g_getenv("GTK_USE_PORTAL"), "1") == 0)
  {
    TestPortalLifecycle(services);
    return;
  }
  TestRestartIgnoresOldRequests(services);
  TestDestroyWithPendingRequests(services);
  TestStopDuringCompassClaim(services);
  TestStopBeforeFirstFix(services);
  TestClientDeactivationAndRetry(services);
  TestDeactivationBeforeFirstFix(services);
  TestInactiveDuringStart(services);
  TestInitialCompassHeading(services);
  TestPermissionFailureAndRetry(services);
  TestCompassFailureKeepsLocation(services);
  TestCompassTimeoutKeepsLocation(services);
  TestCompassHotplugAndRestart(services);
  TestCompassRestartDuringClaim(services);
  TestStopDuringClientStart(services);
  TestSupersededLocationRequest(services);
  TestDestroyWithPendingLocation(services);
  // Cancelled calls complete from the main loop.
  RunFor(100);
}
}  // namespace location_service_tests
