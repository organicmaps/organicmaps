#include "testing/testing.hpp"

#include "map/power_manager/power_manager.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include <functional>
#include <vector>

using namespace platform::tests_support;

struct SubscriberForTesting : public PowerManager::Subscriber
{
public:
  // PowerManager::Subscriber overrides:
  void OnPowerFacilityChanged(PowerManager::Facility const facility, bool enabled) override
  {
    m_onFacilityEvents.push_back({facility, enabled});
  }

  void OnPowerSchemeChanged(PowerManager::Scheme const actualConfig) override
  {
    m_onShemeEvents.push_back(actualConfig);
  }

  struct FacilityState
  {
    PowerManager::Facility m_facility;
    bool m_state;
  };

  std::vector<FacilityState> m_onFacilityEvents;
  std::vector<PowerManager::Scheme> m_onShemeEvents;
};

UNIT_TEST(PowerManager_SetFacility)
{
  ScopedFile sf("power_manager_config", ScopedFile::Mode::DoNotCreate);
  PowerManager manager;
  SubscriberForTesting subscriber;

  manager.Subscribe(&subscriber);

  TEST_EQUAL(manager.IsFacilityEnabled(PowerManager::Facility::Buildings3d), true, ());
  TEST_EQUAL(manager.IsFacilityEnabled(PowerManager::Facility::TrackRecord), true, ());
  TEST_EQUAL(manager.GetScheme(), PowerManager::Scheme::Normal, ());
  manager.SetFacility(PowerManager::Facility::Buildings3d, false);
  TEST_EQUAL(manager.IsFacilityEnabled(PowerManager::Facility::Buildings3d), false, ());
  TEST_EQUAL(manager.IsFacilityEnabled(PowerManager::Facility::TrackRecord), true, ());
  TEST_EQUAL(manager.GetScheme(), PowerManager::Scheme::None, ());

  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_facility,
             PowerManager::Facility::Buildings3d, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_state, false, ());
  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onShemeEvents[0], PowerManager::Scheme::None, ());

  subscriber.m_onFacilityEvents.clear();
  subscriber.m_onShemeEvents.clear();

  manager.SetFacility(PowerManager::Facility::TrackRecord, false);
  TEST_EQUAL(manager.IsFacilityEnabled(PowerManager::Facility::TrackRecord), false, ());
  TEST_EQUAL(manager.GetScheme(), PowerManager::Scheme::Economy, ());

  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_facility, PowerManager::Facility::TrackRecord, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_state, false, ());
  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onShemeEvents[0], PowerManager::Scheme::Economy, ());
}

UNIT_TEST(PowerManager_SetConfig)
{
  ScopedFile sf("power_manager_config", ScopedFile::Mode::DoNotCreate);
  PowerManager manager;
  SubscriberForTesting subscriber;

  manager.Subscribe(&subscriber);

  TEST_EQUAL(manager.GetScheme(), PowerManager::Scheme::Normal, ());
  TEST_EQUAL(manager.IsFacilityEnabled(PowerManager::Facility::Buildings3d), true, ());
  TEST_EQUAL(manager.IsFacilityEnabled(PowerManager::Facility::TrackRecord), true, ());
  manager.SetScheme(PowerManager::Scheme::Economy);
  TEST_EQUAL(manager.GetScheme(), PowerManager::Scheme::Economy, ());
  TEST_EQUAL(manager.IsFacilityEnabled(PowerManager::Facility::Buildings3d), false, ());
  TEST_EQUAL(manager.IsFacilityEnabled(PowerManager::Facility::TrackRecord), false, ());

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onShemeEvents[0], PowerManager::Scheme::Economy, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 2, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_facility,
             PowerManager::Facility::Buildings3d, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_facility, PowerManager::Facility::TrackRecord, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_state, false, ());

  subscriber.m_onFacilityEvents.clear();
  subscriber.m_onShemeEvents.clear();

  manager.SetScheme(PowerManager::Scheme::Normal);
  TEST_EQUAL(manager.GetScheme(), PowerManager::Scheme::Normal, ());
  TEST_EQUAL(manager.IsFacilityEnabled(PowerManager::Facility::Buildings3d), true, ());
  TEST_EQUAL(manager.IsFacilityEnabled(PowerManager::Facility::TrackRecord), true, ());

  TEST_EQUAL(subscriber.m_onShemeEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onShemeEvents[0], PowerManager::Scheme::Normal, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 2, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_facility,
             PowerManager::Facility::Buildings3d, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_state, true, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_facility, PowerManager::Facility::TrackRecord, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_state, true, ());
}
