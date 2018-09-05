#include "testing/testing.hpp"

#include "metrics/eye.hpp"
#include "metrics/eye_info.hpp"
#include "metrics/eye_serdes.hpp"
#include "metrics/eye_storage.hpp"

#include "metrics/metrics_tests_support/eye_for_testing.hpp"

#include <cstdint>
#include <utility>
#include <vector>

using namespace eye;

namespace
{
Info MakeDefaultInfoForTesting()
{
  Info info;
  Tip tip;
  tip.m_type = Tip::Type::DiscoverButton;
  tip.m_eventCounters.Increment(Tip::Event::GotitClicked);
  tip.m_lastShownTime = Time(std::chrono::hours(101010));
  info.m_tips.emplace_back(std::move(tip));

  return info;
}

void CompareWithDefaultInfo(Info const & lhs)
{
  auto const rhs = MakeDefaultInfoForTesting();

  TEST_EQUAL(lhs.m_tips.size(), 1, ());
  TEST_EQUAL(lhs.m_tips.size(), rhs.m_tips.size(), ());
  TEST_EQUAL(lhs.m_tips[0].m_type, rhs.m_tips[0].m_type, ());
  TEST_EQUAL(lhs.m_tips[0].m_lastShownTime, rhs.m_tips[0].m_lastShownTime, ());
  TEST_EQUAL(lhs.m_tips[0].m_eventCounters.Get(Tip::Event::GotitClicked),
             rhs.m_tips[0].m_eventCounters.Get(Tip::Event::GotitClicked), ());
}

Time GetLastShownTipTime(Tips const & tips)
{
  Time lastShownTime;
  for (auto const & tip : tips)
  {
    if (lastShownTime < tip.m_lastShownTime)
      lastShownTime = tip.m_lastShownTime;
  }

  return lastShownTime;
}

Time GetLastShownLayerTime(Layers const & layers)
{
  Time lastUsedTime;
  for (auto const & layer : layers)
  {
    if (lastUsedTime < layer.m_lastTimeUsed)
      lastUsedTime = layer.m_lastTimeUsed;
  }

  return lastUsedTime;
}
}  // namespace

UNIT_TEST(Eye_SerdesTest)
{
  auto const info = MakeDefaultInfoForTesting();

  std::vector<int8_t> s;
  eye::Serdes::Serialize(info, s);
  Info d;
  eye::Serdes::Deserialize(s, d);

  CompareWithDefaultInfo(d);
}

UNIT_CLASS_TEST(ScopedEyeForTesting, SaveLoadTest)
{
  auto const info = MakeDefaultInfoForTesting();

  std::vector<int8_t> s;
  eye::Serdes::Serialize(info, s);
  TEST(eye::Storage::Save(eye::Storage::GetEyeFilePath(), s), ());
  s.clear();
  TEST(eye::Storage::Load(eye::Storage::GetEyeFilePath(), s), ());
  Info d;
  eye::Serdes::Deserialize(s, d);

  CompareWithDefaultInfo(d);
}

UNIT_CLASS_TEST(ScopedEyeForTesting, AppendTipTest)
{
  {
    auto const initialInfo = Eye::Instance().GetInfo();
    auto const & initialTips = initialInfo->m_tips;

    TEST(initialTips.empty(), ());
    TEST_EQUAL(GetLastShownTipTime(initialTips).time_since_epoch().count(), 0, ());
  }
  {
    EyeForTesting::AppendTip(Tip::Type::DiscoverButton, Tip::Event::GotitClicked);

    auto const info = Eye::Instance().GetInfo();
    auto const & tips = info->m_tips;
    auto const lastShownTipTime = GetLastShownTipTime(tips);

    TEST_EQUAL(tips.size(), 1, ());
    TEST_NOT_EQUAL(tips[0].m_lastShownTime.time_since_epoch().count(), 0, ());
    TEST_EQUAL(tips[0].m_type, Tip::Type::DiscoverButton, ());
    TEST_EQUAL(tips[0].m_eventCounters.Get(Tip::Event::GotitClicked), 1, ());
    TEST_EQUAL(tips[0].m_eventCounters.Get(Tip::Event::ActionClicked), 0, ());
    TEST_NOT_EQUAL(lastShownTipTime.time_since_epoch().count(), 0, ());
    TEST_EQUAL(tips[0].m_lastShownTime, lastShownTipTime, ());
  }

  Time prevShowTime;
  {
    EyeForTesting::AppendTip(Tip::Type::PublicTransport, Tip::Event::ActionClicked);

    auto const info = Eye::Instance().GetInfo();
    auto const & tips = info->m_tips;
    auto const lastShownTipTime = GetLastShownTipTime(tips);

    TEST_EQUAL(tips.size(), 2, ());
    TEST_NOT_EQUAL(tips[1].m_lastShownTime.time_since_epoch().count(), 0, ());
    TEST_EQUAL(tips[1].m_type, Tip::Type::PublicTransport, ());
    TEST_EQUAL(tips[1].m_eventCounters.Get(Tip::Event::GotitClicked), 0, ());
    TEST_EQUAL(tips[1].m_eventCounters.Get(Tip::Event::ActionClicked), 1, ());
    TEST_NOT_EQUAL(lastShownTipTime.time_since_epoch().count(), 0, ());
    TEST_EQUAL(tips[1].m_lastShownTime, lastShownTipTime, ());

    prevShowTime = lastShownTipTime;
  }
  {
    EyeForTesting::AppendTip(Tip::Type::PublicTransport, Tip::Event::GotitClicked);

    auto const info = Eye::Instance().GetInfo();
    auto const & tips = info->m_tips;
    auto const lastShownTipTime = GetLastShownTipTime(tips);

    TEST_EQUAL(tips.size(), 2, ());
    TEST_NOT_EQUAL(tips[1].m_lastShownTime.time_since_epoch().count(), 0, ());
    TEST_EQUAL(tips[1].m_type, Tip::Type::PublicTransport, ());
    TEST_EQUAL(tips[1].m_eventCounters.Get(Tip::Event::GotitClicked), 1, ());
    TEST_EQUAL(tips[1].m_eventCounters.Get(Tip::Event::ActionClicked), 1, ());
    TEST_NOT_EQUAL(lastShownTipTime.time_since_epoch().count(), 0, ());
    TEST_EQUAL(tips[1].m_lastShownTime, lastShownTipTime, ());
    TEST_NOT_EQUAL(prevShowTime, lastShownTipTime, ());
  }
}

UNIT_CLASS_TEST(ScopedEyeForTesting, UpdateBookingFilterUsedTimeTest)
{
  auto const initialInfo = Eye::Instance().GetInfo();
  auto const & initialBooking = initialInfo->m_booking;

  TEST_EQUAL(initialBooking.m_lastFilterUsedTime, Time(), ());

  EyeForTesting::UpdateBookingFilterUsedTime();

  auto const info = Eye::Instance().GetInfo();
  auto const & booking = info->m_booking;

  TEST_NOT_EQUAL(initialBooking.m_lastFilterUsedTime, booking.m_lastFilterUsedTime, ());
}

UNIT_CLASS_TEST(ScopedEyeForTesting, UpdateBoomarksCatalogShownTimeTest)
{
  auto const initialInfo = Eye::Instance().GetInfo();
  auto const & initialBookmarks = initialInfo->m_bookmarks;

  TEST_EQUAL(initialBookmarks.m_lastOpenedTime, Time(), ());

  EyeForTesting::UpdateBoomarksCatalogShownTime();

  auto const info = Eye::Instance().GetInfo();
  auto const & bookmarks = info->m_bookmarks;

  TEST_NOT_EQUAL(initialBookmarks.m_lastOpenedTime, bookmarks.m_lastOpenedTime, ());
}

UNIT_CLASS_TEST(ScopedEyeForTesting, UpdateDiscoveryShownTimeTest)
{
  auto const initialInfo = Eye::Instance().GetInfo();
  auto const & initialDiscovery = initialInfo->m_discovery;

  TEST_EQUAL(initialDiscovery.m_lastOpenedTime, Time(), ());

  EyeForTesting::UpdateDiscoveryShownTime();

  auto const info = Eye::Instance().GetInfo();
  auto const & discovery = info->m_discovery;

  TEST_NOT_EQUAL(initialDiscovery.m_lastOpenedTime, discovery.m_lastOpenedTime, ());
}

UNIT_CLASS_TEST(ScopedEyeForTesting, IncrementDiscoveryItemTest)
{
  auto const initialInfo = Eye::Instance().GetInfo();
  auto const & initialDiscovery = initialInfo->m_discovery;

  TEST_EQUAL(initialDiscovery.m_lastClickedTime, Time(), ());
  TEST_EQUAL(initialDiscovery.m_eventCounters.Get(Discovery::Event::AttractionsClicked), 0, ());
  TEST_EQUAL(initialDiscovery.m_eventCounters.Get(Discovery::Event::CafesClicked), 0, ());
  TEST_EQUAL(initialDiscovery.m_eventCounters.Get(Discovery::Event::HotelsClicked), 0, ());
  TEST_EQUAL(initialDiscovery.m_eventCounters.Get(Discovery::Event::LocalsClicked), 0, ());
  TEST_EQUAL(initialDiscovery.m_eventCounters.Get(Discovery::Event::MoreAttractionsClicked), 0, ());
  TEST_EQUAL(initialDiscovery.m_eventCounters.Get(Discovery::Event::MoreCafesClicked), 0, ());
  TEST_EQUAL(initialDiscovery.m_eventCounters.Get(Discovery::Event::MoreHotelsClicked), 0, ());
  TEST_EQUAL(initialDiscovery.m_eventCounters.Get(Discovery::Event::MoreLocalsClicked), 0, ());

  {
    EyeForTesting::IncrementDiscoveryItem(Discovery::Event::CafesClicked);

    auto const info = Eye::Instance().GetInfo();
    auto const & discovery = info->m_discovery;

    TEST_NOT_EQUAL(initialDiscovery.m_lastClickedTime, discovery.m_lastClickedTime, ());

    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::AttractionsClicked), 0, ());
    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::CafesClicked), 1, ());
    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::HotelsClicked), 0, ());
    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::LocalsClicked), 0, ());
    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::MoreAttractionsClicked), 0, ());
    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::MoreCafesClicked), 0, ());
    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::MoreHotelsClicked), 0, ());
    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::MoreLocalsClicked), 0, ());
  }

  {
    EyeForTesting::IncrementDiscoveryItem(Discovery::Event::CafesClicked);

    auto const info = Eye::Instance().GetInfo();
    auto const & discovery = info->m_discovery;

    TEST_NOT_EQUAL(initialDiscovery.m_lastClickedTime, discovery.m_lastClickedTime, ());

    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::AttractionsClicked), 0, ());
    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::CafesClicked), 2, ());
    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::HotelsClicked), 0, ());
    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::LocalsClicked), 0, ());
    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::MoreAttractionsClicked), 0, ());
    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::MoreCafesClicked), 0, ());
    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::MoreHotelsClicked), 0, ());
    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::MoreLocalsClicked), 0, ());
  }

  {
    EyeForTesting::IncrementDiscoveryItem(Discovery::Event::CafesClicked);
    EyeForTesting::IncrementDiscoveryItem(Discovery::Event::HotelsClicked);
    EyeForTesting::IncrementDiscoveryItem(Discovery::Event::MoreLocalsClicked);
    EyeForTesting::IncrementDiscoveryItem(Discovery::Event::MoreHotelsClicked);

    auto const info = Eye::Instance().GetInfo();
    auto const & discovery = info->m_discovery;

    TEST_NOT_EQUAL(initialDiscovery.m_lastClickedTime, discovery.m_lastClickedTime, ());

    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::AttractionsClicked), 0, ());
    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::CafesClicked), 3, ());
    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::HotelsClicked), 1, ());
    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::LocalsClicked), 0, ());
    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::MoreAttractionsClicked), 0, ());
    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::MoreCafesClicked), 0, ());
    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::MoreHotelsClicked), 1, ());
    TEST_EQUAL(discovery.m_eventCounters.Get(Discovery::Event::MoreLocalsClicked), 1, ());
  }
}

UNIT_CLASS_TEST(ScopedEyeForTesting, AppendLayerTest)
{
  {
    auto const initialInfo = Eye::Instance().GetInfo();
    auto const & initialLayers = initialInfo->m_layers;

    TEST(initialLayers.empty(), ());
    TEST_EQUAL(GetLastShownLayerTime(initialLayers), Time(), ());
  }
  Time prevShowTime;
  {
    EyeForTesting::AppendLayer(Layer::Type::PublicTransport);

    auto const info = Eye::Instance().GetInfo();
    auto const & layers = info->m_layers;
    auto const prevShowTime = GetLastShownLayerTime(layers);

    TEST_EQUAL(layers.size(), 1, ());
    TEST_NOT_EQUAL(layers[0].m_lastTimeUsed, Time(), ());
    TEST_EQUAL(layers[0].m_type, Layer::Type::PublicTransport, ());
    TEST_EQUAL(layers[0].m_useCount, 1, ());
    TEST_NOT_EQUAL(prevShowTime, Time(), ());
  }
  {
    EyeForTesting::AppendLayer(Layer::Type::TrafficJams);

    auto const info = Eye::Instance().GetInfo();
    auto const & layers = info->m_layers;
    auto const lastShownLayerTime = GetLastShownLayerTime(layers);

    TEST_EQUAL(layers.size(), 2, ());
    TEST_NOT_EQUAL(layers[1].m_lastTimeUsed, Time(), ());
    TEST_EQUAL(layers[1].m_type, Layer::Type::TrafficJams, ());
    TEST_EQUAL(layers[1].m_useCount, 1, ());
    TEST_EQUAL(layers[1].m_lastTimeUsed, lastShownLayerTime, ());
    TEST_NOT_EQUAL(prevShowTime, lastShownLayerTime, ());
    prevShowTime = lastShownLayerTime;
  }
  {
    EyeForTesting::AppendLayer(Layer::Type::TrafficJams);

    auto const info = Eye::Instance().GetInfo();
    auto const & layers = info->m_layers;
    auto const lastShownLayerTime = GetLastShownLayerTime(layers);

    TEST_EQUAL(layers.size(), 2, ());
    TEST_NOT_EQUAL(layers[1].m_lastTimeUsed, Time(), ());
    TEST_EQUAL(layers[1].m_type, Layer::Type::TrafficJams, ());
    TEST_EQUAL(layers[1].m_useCount, 2, ());
    TEST_EQUAL(layers[1].m_lastTimeUsed, lastShownLayerTime, ());
    TEST_NOT_EQUAL(prevShowTime, lastShownLayerTime, ());
  }
}
