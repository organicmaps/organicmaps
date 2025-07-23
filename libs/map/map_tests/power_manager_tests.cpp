#include "testing/testing.hpp"

#include "map/power_management/power_management_schemas.hpp"
#include "map/power_management/power_manager.hpp"

#include "platform//platform.hpp"

#include "coding/file_writer.hpp"

#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"

#include <functional>
#include <vector>

using namespace power_management;

namespace
{
struct SubscriberForTesting : public PowerManager::Subscriber
{
public:
  // PowerManager::Subscriber overrides:
  void OnPowerFacilityChanged(Facility const facility, bool enabled) override
  {
    m_onFacilityEvents.push_back({facility, enabled});
  }

  void OnPowerSchemeChanged(Scheme const actualConfig) override { m_onShemeEvents.push_back(actualConfig); }

  struct FacilityState
  {
    Facility m_facility;
    bool m_state;
  };

  std::vector<FacilityState> m_onFacilityEvents;
  std::vector<Scheme> m_onShemeEvents;
};

void TestIsAllFacilitiesInState(PowerManager const & manager, bool state)
{
  auto const count = static_cast<size_t>(Facility::Count);
  for (size_t i = 0; i < count; ++i)
    TEST_EQUAL(manager.IsFacilityEnabled(static_cast<Facility>(i)), state, ());
}

void TestAllFacilitiesEnabledExcept(PowerManager const & manager, std::vector<Facility> const & disabledFacilities)
{
  auto const count = static_cast<size_t>(Facility::Count);
  for (size_t i = 0; i < count; ++i)
  {
    auto const facility = static_cast<Facility>(i);
    TEST_EQUAL(manager.IsFacilityEnabled(facility), !base::IsExist(disabledFacilities, facility), ());
  }
}

UNIT_TEST(PowerManager_SetFacility)
{
  auto const configPath = PowerManager::GetConfigPath();
  SCOPE_GUARD(deleteFileGuard, bind(&FileWriter::DeleteFileX, std::cref(configPath)));
  PowerManager manager;
  SubscriberForTesting subscriber;

  manager.Subscribe(&subscriber);

  TestIsAllFacilitiesInState(manager, true);
  TEST_EQUAL(manager.GetScheme(), Scheme::Normal, ());
  manager.SetFacility(Facility::Buildings3d, false);
  TestAllFacilitiesEnabledExcept(manager, {Facility::Buildings3d});
  TEST_EQUAL(manager.GetScheme(), Scheme::None, ());

  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_facility, Facility::Buildings3d, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_state, false, ());
  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onShemeEvents[0], Scheme::None, ());

  subscriber.m_onFacilityEvents.clear();
  subscriber.m_onShemeEvents.clear();

  manager.SetFacility(Facility::MapDownloader, false);
  TestAllFacilitiesEnabledExcept(manager, {Facility::Buildings3d, Facility::MapDownloader});
  TEST_EQUAL(manager.GetScheme(), Scheme::None, ());

  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_facility, Facility::MapDownloader, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_state, false, ());
  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 0, ());
}

UNIT_TEST(PowerManager_SetScheme)
{
  auto const configPath = PowerManager::GetConfigPath();
  SCOPE_GUARD(deleteFileGuard, bind(&FileWriter::DeleteFileX, std::cref(configPath)));
  Platform::ThreadRunner m_runner;
  PowerManager manager;
  SubscriberForTesting subscriber;

  manager.Subscribe(&subscriber);

  TestIsAllFacilitiesInState(manager, true);
  TEST_EQUAL(manager.GetScheme(), Scheme::Normal, ());
  manager.SetScheme(Scheme::EconomyMaximum);
  TEST_EQUAL(manager.GetScheme(), Scheme::EconomyMaximum, ());
  TestAllFacilitiesEnabledExcept(
      manager, {Facility::Buildings3d, Facility::PerspectiveView, Facility::TrackRecording, Facility::TrafficJams,
                Facility::GpsTrackingForTraffic, Facility::OsmEditsUploading, Facility::UgcUploading,
                Facility::BookmarkCloudUploading});

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onShemeEvents[0], Scheme::EconomyMaximum, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 8, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_facility, Facility::Buildings3d, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_facility, Facility::PerspectiveView, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[2].m_facility, Facility::TrackRecording, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[2].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[3].m_facility, Facility::TrafficJams, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[3].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[4].m_facility, Facility::GpsTrackingForTraffic, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[4].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[5].m_facility, Facility::OsmEditsUploading, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[5].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[6].m_facility, Facility::UgcUploading, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[6].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[7].m_facility, Facility::BookmarkCloudUploading, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[7].m_state, false, ());

  subscriber.m_onFacilityEvents.clear();
  subscriber.m_onShemeEvents.clear();

  manager.SetScheme(Scheme::EconomyMedium);
  TEST_EQUAL(manager.GetScheme(), Scheme::EconomyMedium, ());

  TestAllFacilitiesEnabledExcept(manager, {Facility::PerspectiveView, Facility::BookmarkCloudUploading});

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onShemeEvents[0], Scheme::EconomyMedium, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 6, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_facility, Facility::Buildings3d, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_state, true, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_facility, Facility::TrackRecording, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_state, true, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[2].m_facility, Facility::TrafficJams, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[2].m_state, true, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[3].m_facility, Facility::GpsTrackingForTraffic, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[3].m_state, true, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[4].m_facility, Facility::OsmEditsUploading, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[4].m_state, true, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[5].m_facility, Facility::UgcUploading, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[5].m_state, true, ());

  subscriber.m_onShemeEvents.clear();
  subscriber.m_onFacilityEvents.clear();

  manager.SetScheme(Scheme::Auto);

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onShemeEvents[0], Scheme::Auto, ());

  subscriber.m_onShemeEvents.clear();
  subscriber.m_onFacilityEvents.clear();

  manager.SetScheme(Scheme::Normal);

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onShemeEvents[0], Scheme::Normal, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 0, ());
}

UNIT_TEST(PowerManager_OnBatteryLevelChanged)
{
  auto const configPath = PowerManager::GetConfigPath();
  SCOPE_GUARD(deleteFileGuard, bind(&FileWriter::DeleteFileX, std::cref(configPath)));
  Platform::ThreadRunner m_runner;
  PowerManager manager;
  SubscriberForTesting subscriber;

  manager.Subscribe(&subscriber);

  TestIsAllFacilitiesInState(manager, true);
  TEST_EQUAL(manager.GetScheme(), Scheme::Normal, ());

  manager.OnBatteryLevelReceived(50);

  TestIsAllFacilitiesInState(manager, true);
  TEST_EQUAL(manager.GetScheme(), Scheme::Normal, ());

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 0, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 0, ());

  manager.OnBatteryLevelReceived(10);

  TestIsAllFacilitiesInState(manager, true);
  TEST_EQUAL(manager.GetScheme(), Scheme::Normal, ());

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 0, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 0, ());

  manager.SetScheme(Scheme::Auto);

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onShemeEvents[0], Scheme::Auto, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 0, ());

  subscriber.m_onShemeEvents.clear();
  subscriber.m_onFacilityEvents.clear();

  manager.OnBatteryLevelReceived(50);

  TestIsAllFacilitiesInState(manager, true);
  TEST_EQUAL(manager.GetScheme(), Scheme::Auto, ());

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 0, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 0, ());

  manager.OnBatteryLevelReceived(28);

  TestAllFacilitiesEnabledExcept(manager, {Facility::PerspectiveView, Facility::GpsTrackingForTraffic,
                                           Facility::BookmarkCloudUploading, Facility::MapDownloader});
  TEST_EQUAL(manager.GetScheme(), Scheme::Auto, ());

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 0, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 4, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_facility, Facility::PerspectiveView, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_facility, Facility::GpsTrackingForTraffic, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[2].m_facility, Facility::BookmarkCloudUploading, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[2].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[3].m_facility, Facility::MapDownloader, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[3].m_state, false, ());

  subscriber.m_onShemeEvents.clear();
  subscriber.m_onFacilityEvents.clear();

  manager.OnBatteryLevelReceived(10);

  TestIsAllFacilitiesInState(manager, false);
  TEST_EQUAL(manager.GetScheme(), Scheme::Auto, ());

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 0, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 5, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_facility, Facility::Buildings3d, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_facility, Facility::TrackRecording, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[2].m_facility, Facility::TrafficJams, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[2].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[3].m_facility, Facility::OsmEditsUploading, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[3].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[4].m_facility, Facility::UgcUploading, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[4].m_state, false, ());

  subscriber.m_onShemeEvents.clear();
  subscriber.m_onFacilityEvents.clear();

  manager.OnBatteryLevelReceived(100);

  TestIsAllFacilitiesInState(manager, true);
  TEST_EQUAL(manager.GetScheme(), Scheme::Auto, ());

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 0, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 9, ());

  auto const & facilityEvents = subscriber.m_onFacilityEvents;
  for (size_t i = 0; i < facilityEvents.size(); ++i)
  {
    TEST_EQUAL(facilityEvents[i].m_facility, static_cast<Facility>(i), ());
    TEST_EQUAL(subscriber.m_onFacilityEvents[i].m_state, true, ());
  }
}
}  // namespace
