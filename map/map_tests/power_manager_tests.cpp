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
  void OnFacilityStateChanged(PowerManager::Facility const facility, bool state) override
  {
    m_onFacilityEvents.push_back({facility, state});
  }

  void OnConfigChanged(PowerManager::Config const actualConfig) override
  {
    m_onConfigEvents.push_back(actualConfig);
  }

  struct facilityState
  {
    PowerManager::Facility m_facility;
    bool m_state;
  };

  std::vector<facilityState> m_onFacilityEvents;
  std::vector<PowerManager::Config> m_onConfigEvents;
};

UNIT_TEST(PowerManager_SetFacility)
{
  ScopedFile sf("power_manager_config", ScopedFile::Mode::DoNotCreate);
  PowerManager manager;
  SubscriberForTesting subscriber;

  manager.Subscribe(&subscriber);

  TEST_EQUAL(manager.GetFacility(PowerManager::Facility::ThreeDimensionalBuildings), true, ());
  TEST_EQUAL(manager.GetFacility(PowerManager::Facility::TrackRecord), true, ());
  TEST_EQUAL(manager.GetConfig(), PowerManager::Config::Normal, ());
  manager.SetFacility(PowerManager::Facility::ThreeDimensionalBuildings, false);
  TEST_EQUAL(manager.GetFacility(PowerManager::Facility::ThreeDimensionalBuildings), false, ());
  TEST_EQUAL(manager.GetFacility(PowerManager::Facility::TrackRecord), true, ());
  TEST_EQUAL(manager.GetConfig(), PowerManager::Config::None, ());

  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_facility,
             PowerManager::Facility::ThreeDimensionalBuildings, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_state, false, ());
  TEST_EQUAL(subscriber.m_onConfigEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onConfigEvents[0], PowerManager::Config::None, ());

  subscriber.m_onFacilityEvents.clear();
  subscriber.m_onConfigEvents.clear();

  manager.SetFacility(PowerManager::Facility::TrackRecord, false);
  TEST_EQUAL(manager.GetFacility(PowerManager::Facility::TrackRecord), false, ());
  TEST_EQUAL(manager.GetConfig(), PowerManager::Config::Economy, ());

  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_facility, PowerManager::Facility::TrackRecord, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_state, false, ());
  TEST_EQUAL(subscriber.m_onConfigEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onConfigEvents[0], PowerManager::Config::Economy, ());
}

UNIT_TEST(PowerManager_SetConfig)
{
  ScopedFile sf("power_manager_config", ScopedFile::Mode::DoNotCreate);
  PowerManager manager;
  SubscriberForTesting subscriber;

  manager.Subscribe(&subscriber);

  TEST_EQUAL(manager.GetConfig(), PowerManager::Config::Normal, ());
  TEST_EQUAL(manager.GetFacility(PowerManager::Facility::ThreeDimensionalBuildings), true, ());
  TEST_EQUAL(manager.GetFacility(PowerManager::Facility::TrackRecord), true, ());
  manager.SetConfig(PowerManager::Config::Economy);
  TEST_EQUAL(manager.GetConfig(), PowerManager::Config::Economy, ());
  TEST_EQUAL(manager.GetFacility(PowerManager::Facility::ThreeDimensionalBuildings), false, ());
  TEST_EQUAL(manager.GetFacility(PowerManager::Facility::TrackRecord), false, ());

  TEST_EQUAL(subscriber.m_onConfigEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onConfigEvents[0], PowerManager::Config::Economy, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 2, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_facility,
             PowerManager::Facility::ThreeDimensionalBuildings, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_state, false, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_facility, PowerManager::Facility::TrackRecord, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_state, false, ());

  subscriber.m_onFacilityEvents.clear();
  subscriber.m_onConfigEvents.clear();

  manager.SetConfig(PowerManager::Config::Normal);
  TEST_EQUAL(manager.GetConfig(), PowerManager::Config::Normal, ());
  TEST_EQUAL(manager.GetFacility(PowerManager::Facility::ThreeDimensionalBuildings), true, ());
  TEST_EQUAL(manager.GetFacility(PowerManager::Facility::TrackRecord), true, ());

  TEST_EQUAL(subscriber.m_onConfigEvents.size(), 1, ());
  TEST_EQUAL(subscriber.m_onConfigEvents[0], PowerManager::Config::Normal, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents.size(), 2, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_facility,
             PowerManager::Facility::ThreeDimensionalBuildings, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[0].m_state, true, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_facility, PowerManager::Facility::TrackRecord, ());
  TEST_EQUAL(subscriber.m_onFacilityEvents[1].m_state, true, ());
}
