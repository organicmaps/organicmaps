#include "testing/testing.hpp"

#include "map/power_management/power_management_schemas.hpp"
#include "map/power_management/power_manager.hpp"

#include "platform//platform.hpp"

#include "coding/file_writer.hpp"

#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"

#include <functional>
#include <vector>

namespace
{
struct SubscriberForTesting : public power_management::PowerManager::Subscriber
{
public:
  // PowerManager::Subscriber overrides:
  void OnPowerFacilityChanged(power_management::Facility const facility, bool enabled) override
  {
    m_onFacilityEvents.push_back({facility, enabled});
  }

  void OnPowerSchemeChanged(power_management::Scheme const actualConfig) override
  {
    m_onShemeEvents.push_back(actualConfig);
  }

  struct FacilityState
  {
    power_management::Facility m_facility;
    bool m_state;
  };

  std::vector<FacilityState> m_onFacilityEvents;
  std::vector<power_management::Scheme> m_onShemeEvents;
};

void TestIsAllFacilitiesInState(power_management::PowerManager const & manager, bool state)
{
  auto const count = static_cast<size_t>(power_management::Facility::Count);
  for (size_t i = 0; i < count; ++i)
    TEST_EQUAL(manager.IsFacilityEnabled(static_cast<power_management::Facility>(i)), state, ());
}

void TestAllFacilitiesEnabledExcept(power_management::PowerManager const & manager,
                                    std::vector<power_management::Facility> const & disabledFacilities)
{
  auto const count = static_cast<size_t>(power_management::Facility::Count);
  for (size_t i = 0; i < count; ++i)
  {
    auto const facility = static_cast<power_management::Facility>(i);
    TEST_EQUAL(manager.IsFacilityEnabled(facility), !base::IsExist(disabledFacilities, facility), ());
  }
}

UNIT_TEST(PowerManager_SetFacility)
{
  auto const configPath = power_management::PowerManager::GetConfigPath();
  SCOPE_GUARD(deleteFileGuard, std::bind(&FileWriter::DeleteFileX, std::cref(configPath)));
  power_management::PowerManager manager;
  SubscriberForTesting subscriber;

  manager.Subscribe(&subscriber);

  TestIsAllFacilitiesInState(manager, true);
  TEST_EQUAL(manager.GetScheme(), power_management::Scheme::Normal, ());
  manager.SetFacility(power_management::Facility::Buildings3d, false);
  TestAllFacilitiesEnabledExcept(manager, {power_management::Facility::Buildings3d});
  TEST_EQUAL(manager.GetScheme(), power_management::Scheme::None, ());

  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_facility, power_management::Facility::Buildings3d, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_state, false, ());
  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onShemeEvents[0], power_management::Scheme::None, ());

  subscriber.m_onFacilityEvents.clear();
  subscriber.m_onShemeEvents.clear();

  manager.SetFacility(power_management::Facility::MapDownloader, false);
  TestAllFacilitiesEnabledExcept(manager,
                                 {power_management::Facility::Buildings3d, power_management::Facility::MapDownloader});
  TEST_EQUAL(manager.GetScheme(), power_management::Scheme::None, ());

  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_facility, power_management::Facility::MapDownloader, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_state, false, ());
  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 0, ());
}

UNIT_TEST(PowerManager_SetScheme)
{
  auto const configPath = power_management::PowerManager::GetConfigPath();
  SCOPE_GUARD(deleteFileGuard, std::bind(&FileWriter::DeleteFileX, std::cref(configPath)));
  Platform::ThreadRunner m_runner;
  power_management::PowerManager manager;
  SubscriberForTesting subscriber;

  manager.Subscribe(&subscriber);

  TestIsAllFacilitiesInState(manager, true);
  TEST_EQUAL(manager.GetScheme(), power_management::Scheme::Normal, ());
  manager.SetScheme(power_management::Scheme::EconomyMaximum);
  TEST_EQUAL(manager.GetScheme(), power_management::Scheme::EconomyMaximum, ());
  TestAllFacilitiesEnabledExcept(
      manager, {power_management::Facility::Buildings3d, power_management::Facility::PerspectiveView,
                power_management::Facility::TrackRecording, power_management::Facility::TrafficJams,
                power_management::Facility::GpsTrackingForTraffic, power_management::Facility::OsmEditsUploading,
                power_management::Facility::UgcUploading, power_management::Facility::BookmarkCloudUploading});

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onShemeEvents[0], power_management::Scheme::EconomyMaximum, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 8, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_facility, power_management::Facility::Buildings3d, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_facility, power_management::Facility::PerspectiveView, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[2].m_facility, power_management::Facility::TrackRecording, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[2].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[3].m_facility, power_management::Facility::TrafficJams, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[3].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[4].m_facility, power_management::Facility::GpsTrackingForTraffic, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[4].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[5].m_facility, power_management::Facility::OsmEditsUploading, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[5].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[6].m_facility, power_management::Facility::UgcUploading, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[6].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[7].m_facility, power_management::Facility::BookmarkCloudUploading, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[7].m_state, false, ());

  subscriber.m_onFacilityEvents.clear();
  subscriber.m_onShemeEvents.clear();

  manager.SetScheme(power_management::Scheme::EconomyMedium);
  TEST_EQUAL(manager.GetScheme(), power_management::Scheme::EconomyMedium, ());

  TestAllFacilitiesEnabledExcept(
      manager, {power_management::Facility::PerspectiveView, power_management::Facility::BookmarkCloudUploading});

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onShemeEvents[0], power_management::Scheme::EconomyMedium, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 6, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_facility, power_management::Facility::Buildings3d, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_state, true, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_facility, power_management::Facility::TrackRecording, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_state, true, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[2].m_facility, power_management::Facility::TrafficJams, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[2].m_state, true, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[3].m_facility, power_management::Facility::GpsTrackingForTraffic, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[3].m_state, true, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[4].m_facility, power_management::Facility::OsmEditsUploading, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[4].m_state, true, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[5].m_facility, power_management::Facility::UgcUploading, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[5].m_state, true, ());

  subscriber.m_onShemeEvents.clear();
  subscriber.m_onFacilityEvents.clear();

  manager.SetScheme(power_management::Scheme::Auto);

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onShemeEvents[0], power_management::Scheme::Auto, ());

  subscriber.m_onShemeEvents.clear();
  subscriber.m_onFacilityEvents.clear();

  manager.SetScheme(power_management::Scheme::Normal);

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onShemeEvents[0], power_management::Scheme::Normal, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 0, ());
}

UNIT_TEST(PowerManager_OnBatteryLevelChanged)
{
  auto const configPath = power_management::PowerManager::GetConfigPath();
  SCOPE_GUARD(deleteFileGuard, std::bind(&FileWriter::DeleteFileX, std::cref(configPath)));
  Platform::ThreadRunner m_runner;
  power_management::PowerManager manager;
  SubscriberForTesting subscriber;

  manager.Subscribe(&subscriber);

  TestIsAllFacilitiesInState(manager, true);
  TEST_EQUAL(manager.GetScheme(), power_management::Scheme::Normal, ());

  manager.OnBatteryLevelReceived(50);

  TestIsAllFacilitiesInState(manager, true);
  TEST_EQUAL(manager.GetScheme(), power_management::Scheme::Normal, ());

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 0, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 0, ());

  manager.OnBatteryLevelReceived(10);

  TestIsAllFacilitiesInState(manager, true);
  TEST_EQUAL(manager.GetScheme(), power_management::Scheme::Normal, ());

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 0, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 0, ());

  manager.SetScheme(power_management::Scheme::Auto);

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onShemeEvents[0], power_management::Scheme::Auto, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 0, ());

  subscriber.m_onShemeEvents.clear();
  subscriber.m_onFacilityEvents.clear();

  manager.OnBatteryLevelReceived(50);

  TestIsAllFacilitiesInState(manager, true);
  TEST_EQUAL(manager.GetScheme(), power_management::Scheme::Auto, ());

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 0, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 0, ());

  manager.OnBatteryLevelReceived(28);

  TestAllFacilitiesEnabledExcept(
      manager, {power_management::Facility::PerspectiveView, power_management::Facility::GpsTrackingForTraffic,
                power_management::Facility::BookmarkCloudUploading, power_management::Facility::MapDownloader});
  TEST_EQUAL(manager.GetScheme(), power_management::Scheme::Auto, ());

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 0, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 4, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_facility, power_management::Facility::PerspectiveView, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_facility, power_management::Facility::GpsTrackingForTraffic, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[2].m_facility, power_management::Facility::BookmarkCloudUploading, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[2].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[3].m_facility, power_management::Facility::MapDownloader, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[3].m_state, false, ());

  subscriber.m_onShemeEvents.clear();
  subscriber.m_onFacilityEvents.clear();

  manager.OnBatteryLevelReceived(10);

  TestIsAllFacilitiesInState(manager, false);
  TEST_EQUAL(manager.GetScheme(), power_management::Scheme::Auto, ());

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 0, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 5, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_facility, power_management::Facility::Buildings3d, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_facility, power_management::Facility::TrackRecording, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[2].m_facility, power_management::Facility::TrafficJams, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[2].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[3].m_facility, power_management::Facility::OsmEditsUploading, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[3].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[4].m_facility, power_management::Facility::UgcUploading, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[4].m_state, false, ());

  subscriber.m_onShemeEvents.clear();
  subscriber.m_onFacilityEvents.clear();

  manager.OnBatteryLevelReceived(100);

  TestIsAllFacilitiesInState(manager, true);
  TEST_EQUAL(manager.GetScheme(), power_management::Scheme::Auto, ());

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 0, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 9, ());

  auto const & facilityEvents = subscriber.m_onFacilityEvents;
  for (size_t i = 0; i < facilityEvents.size(); ++i)
  {
    TEST_EQUAL(facilityEvents[i].m_facility, static_cast<power_management::Facility>(i), ());
    TEST_EQUAL(subscriber.m_onFacilityEvents[i].m_state, true, ());
  }
}
}  // namespace
