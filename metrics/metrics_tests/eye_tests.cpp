#include "testing/testing.hpp"

#include "metrics/eye.hpp"
#include "metrics/eye_info.hpp"
#include "metrics/eye_serdes.hpp"
#include "metrics/eye_storage.hpp"

#include "metrics/metrics_tests_support/eye_for_testing.hpp"

#include <chrono>
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

  info.m_booking.m_lastFilterUsedTime = Time(std::chrono::minutes(100100));

  info.m_bookmarks.m_lastOpenedTime = Time(std::chrono::minutes(10000));

  Layer layer;
  layer.m_useCount = 3;
  layer.m_lastTimeUsed = Time(std::chrono::hours(20000));
  layer.m_type = Layer::Type::PublicTransport;
  info.m_layers.emplace_back(std::move(layer));

  info.m_discovery.m_lastOpenedTime = Time(std::chrono::hours(30000));
  info.m_discovery.m_eventCounters.Increment(Discovery::Event::MoreAttractionsClicked);
  info.m_discovery.m_lastClickedTime = Time(std::chrono::hours(30005));

  MapObject poi;
  poi.SetBestType("shop");
  poi.SetPos({53.652007, 108.143443});
  poi.SetDefaultName("Hello");
  poi.SetReadableName("World");
  MapObject::Event eventInfo;
  eventInfo.m_eventTime = Time(std::chrono::hours(90000));
  eventInfo.m_userPos = {72.045507, 81.408095};
  eventInfo.m_type = MapObject::Event::Type::AddToBookmark;
  poi.GetEditableEvents().push_back(eventInfo);
  eventInfo.m_eventTime = Time(std::chrono::hours(80000));
  eventInfo.m_userPos = {53.016347, 158.683327};
  eventInfo.m_type = MapObject::Event::Type::Open;
  poi.GetEditableEvents().push_back(eventInfo);
  info.m_mapObjects.Add(poi);

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
  TEST_EQUAL(lhs.m_booking.m_lastFilterUsedTime, rhs.m_booking.m_lastFilterUsedTime, ());
  TEST_EQUAL(lhs.m_bookmarks.m_lastOpenedTime, rhs.m_bookmarks.m_lastOpenedTime, ());
  TEST_EQUAL(lhs.m_layers.size(), rhs.m_layers.size(), ());
  TEST_EQUAL(lhs.m_layers.back().m_type, rhs.m_layers.back().m_type, ());
  TEST_EQUAL(lhs.m_layers.back().m_lastTimeUsed, rhs.m_layers.back().m_lastTimeUsed, ());
  TEST_EQUAL(lhs.m_layers.back().m_useCount, rhs.m_layers.back().m_useCount, ());
  TEST_EQUAL(lhs.m_discovery.m_lastOpenedTime, rhs.m_discovery.m_lastOpenedTime, ());
  TEST_EQUAL(lhs.m_discovery.m_lastClickedTime, rhs.m_discovery.m_lastClickedTime, ());
  TEST_EQUAL(lhs.m_discovery.m_eventCounters.Get(Discovery::Event::MoreAttractionsClicked),
             rhs.m_discovery.m_eventCounters.Get(Discovery::Event::MoreAttractionsClicked), ());
  TEST_EQUAL(lhs.m_mapObjects.GetSize(), rhs.m_mapObjects.GetSize(), ());

  lhs.m_mapObjects.ForEach([&rhs](MapObject const & lhsObj)
  {
    rhs.m_mapObjects.ForEach([&lhsObj](MapObject const & rhsObj)
    {
      TEST(lhsObj.GetPos().EqualDxDy(rhsObj.GetPos(), 1e-6), ());
      TEST_EQUAL(lhsObj.GetBestType(), rhsObj.GetBestType(), ());
      TEST_EQUAL(lhsObj.GetDefaultName(), rhsObj.GetDefaultName(), ());
      TEST_EQUAL(lhsObj.GetReadableName(), rhsObj.GetReadableName(), ());
      TEST_EQUAL(lhsObj.GetEvents().size(), rhsObj.GetEvents().size(), ());
      TEST(lhsObj.GetEvents()[0].m_userPos.EqualDxDy(rhsObj.GetEvents()[0].m_userPos, 1e-6), ());
      TEST_EQUAL(lhsObj.GetEvents()[0].m_eventTime, rhsObj.GetEvents()[0].m_eventTime, ());
      TEST_EQUAL(lhsObj.GetEvents()[0].m_type, rhsObj.GetEvents()[0].m_type, ());
      TEST(lhsObj.GetEvents()[1].m_userPos.EqualDxDy(rhsObj.GetEvents()[1].m_userPos, 1e-6), ());
      TEST_EQUAL(lhsObj.GetEvents()[1].m_eventTime, rhsObj.GetEvents()[1].m_eventTime, ());
      TEST_EQUAL(lhsObj.GetEvents()[1].m_type, rhsObj.GetEvents()[1].m_type, ());
    });
  });
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

  std::vector<int8_t> infoData;
  std::vector<int8_t> mapObjectsData;
  eye::Serdes::SerializeInfo(info, infoData);
  eye::Serdes::SerializeMapObjects(info.m_mapObjects, mapObjectsData);
  Info result;
  eye::Serdes::DeserializeInfo(infoData, result);
  eye::Serdes::DeserializeMapObjects(mapObjectsData, result.m_mapObjects);

  CompareWithDefaultInfo(result);
}

UNIT_CLASS_TEST(ScopedEyeForTesting, SaveLoadTest)
{
  auto const info = MakeDefaultInfoForTesting();

  std::vector<int8_t> infoData;
  std::vector<int8_t> mapObjectsData;
  eye::Serdes::SerializeInfo(info, infoData);
  eye::Serdes::SerializeMapObjects(info.m_mapObjects, mapObjectsData);
  TEST(eye::Storage::SaveInfo(infoData), ());
  TEST(eye::Storage::SaveMapObjects(mapObjectsData), ());
  infoData.clear();
  mapObjectsData.clear();
  TEST(eye::Storage::LoadInfo(infoData), ());
  TEST(eye::Storage::LoadMapObjects(mapObjectsData), ());
  Info result;
  eye::Serdes::DeserializeInfo(infoData, result);
  eye::Serdes::DeserializeMapObjects(mapObjectsData, result.m_mapObjects);

  CompareWithDefaultInfo(result);
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

UNIT_CLASS_TEST(ScopedEyeForTesting, TrimExpiredMapObjectEvents)
{
  Info info;
  {
    MapObject poi;
    poi.SetBestType("shop");
    poi.SetPos({53.652007, 108.143443});
    MapObject::Event eventInfo;

    eventInfo.m_eventTime = Clock::now() - std::chrono::hours((24 * 30 * 3) + 1);
    eventInfo.m_userPos = {72.045507, 81.408095};
    eventInfo.m_type = MapObject::Event::Type::Open;
    poi.GetEditableEvents().emplace_back(eventInfo);

    eventInfo.m_eventTime =
        Clock::now() - (std::chrono::hours(24 * 30 * 3) + std::chrono::seconds(1));
    eventInfo.m_userPos = {72.045400, 81.408200};
    eventInfo.m_type = MapObject::Event::Type::AddToBookmark;
    poi.GetEditableEvents().emplace_back(eventInfo);

    eventInfo.m_eventTime = Clock::now() - std::chrono::hours(24 * 30 * 3);
    eventInfo.m_userPos = {72.045450, 81.408201};
    eventInfo.m_type = MapObject::Event::Type::RouteToCreated;
    poi.GetEditableEvents().emplace_back(eventInfo);

    info.m_mapObjects.Add(poi);
  }

  {
    MapObject poi;
    poi.SetBestType("cafe");
    poi.SetPos({53.652005, 108.143448});
    MapObject::Event eventInfo;

    eventInfo.m_eventTime = Clock::now() - std::chrono::hours(24 * 30 * 3);
    eventInfo.m_userPos = {53.016347, 158.683327};
    eventInfo.m_type = MapObject::Event::Type::Open;
    poi.GetEditableEvents().emplace_back(eventInfo);

    eventInfo.m_eventTime = Clock::now() - std::chrono::seconds(30);
    eventInfo.m_userPos = {53.016347, 158.683327};
    eventInfo.m_type = MapObject::Event::Type::UgcEditorOpened;
    poi.GetEditableEvents().emplace_back(eventInfo);

    eventInfo.m_eventTime = Clock::now();
    eventInfo.m_userPos = {53.116347, 158.783327};
    eventInfo.m_type = MapObject::Event::Type::UgcSaved;
    poi.GetEditableEvents().emplace_back(eventInfo);

    info.m_mapObjects.Add(poi);
  }

  EyeForTesting::SetInfo(info);

  {
    auto const resultInfo = Eye::Instance().GetInfo();
    auto const & mapObjects = resultInfo->m_mapObjects;
    TEST_EQUAL(mapObjects.GetSize(), 2, ());

    {
      MapObject poi;
      poi.SetBestType("shop");
      poi.SetPos({53.652007, 108.143443});

      bool found = false;
      mapObjects.ForEachInRect(poi.GetLimitRect(), [&poi, &found](MapObject const & item)
      {
        if (poi != item)
          return;

        if (!found)
          found = true;

        TEST_EQUAL(item.GetEvents().size(), 3, ());
        TEST_EQUAL(item.GetEvents()[0].m_type, MapObject::Event::Type::Open, ());
        TEST_EQUAL(item.GetEvents()[1].m_userPos, m2::PointD(72.045400, 81.408200), ());
        TEST_EQUAL(item.GetEvents()[2].m_userPos, m2::PointD(72.045450, 81.408201), ());
      });

      TEST(found, ());
    }

    {
      MapObject poi;
      poi.SetBestType("cafe");
      poi.SetPos({53.652005, 108.143448});

      bool found = false;
      mapObjects.ForEachInRect(poi.GetLimitRect(), [&poi, &found](MapObject const & item)
      {
        if (poi != item)
          return;

        if (!found)
          found = true;

        TEST_EQUAL(item.GetEvents().size(), 3, ());
        TEST_EQUAL(item.GetEvents()[0].m_type, MapObject::Event::Type::Open, ());
        TEST_EQUAL(item.GetEvents()[1].m_userPos, m2::PointD(53.016347, 158.683327), ());
        TEST_EQUAL(item.GetEvents()[2].m_userPos, m2::PointD(53.116347, 158.783327), ());
      });

      TEST(found, ());
    }
  }

  EyeForTesting::TrimExpiredMapObjectEvents();

  {
    auto const resultInfo = Eye::Instance().GetInfo();
    auto const & mapObjects = resultInfo->m_mapObjects;
    TEST_EQUAL(mapObjects.GetSize(), 1, ());

    {
      MapObject poi;
      poi.SetBestType("shop");
      poi.SetPos({53.652007, 108.143443});

      bool found = false;
      mapObjects.ForEachInRect(poi.GetLimitRect(), [&poi, &found](MapObject const & item)
      {
        if (poi != item)
          return;

        if (!found)
          found = true;
      });

      TEST(!found, ());
    }

    {
      MapObject poi;
      poi.SetBestType("cafe");
      poi.SetPos({53.652005, 108.143448});

      bool found = false;
      mapObjects.ForEachInRect(poi.GetLimitRect(), [&poi, &found](MapObject const & item)
      {
        if (poi != item)
          return;

        if (!found)
          found = true;

        TEST_EQUAL(item.GetEvents().size(), 2, ());
        TEST_EQUAL(item.GetEvents()[0].m_userPos, m2::PointD(53.016347, 158.683327), ());
        TEST_EQUAL(item.GetEvents()[0].m_type, MapObject::Event::Type::UgcEditorOpened, ());

        TEST_EQUAL(item.GetEvents()[1].m_userPos, m2::PointD(53.116347, 158.783327), ());
        TEST_EQUAL(item.GetEvents()[1].m_type, MapObject::Event::Type::UgcSaved, ());
      });

      TEST(found, ());
    }
  }
}

UNIT_CLASS_TEST(ScopedEyeForTesting, RegisterMapObjectEvent)
{
  {
    MapObject poi;
    poi.SetBestType("cafe");
    poi.SetPos({53.652005, 108.143448});
    m2::PointD userPos = {53.016347, 158.683327};

    EyeForTesting::RegisterMapObjectEvent(poi, MapObject::Event::Type::Open, userPos);

    userPos = {53.016345, 158.683329};

    EyeForTesting::RegisterMapObjectEvent(poi, MapObject::Event::Type::RouteToCreated, userPos);
  }
  {
    MapObject poi;
    poi.SetBestType("shop");
    poi.SetPos({53.652005, 108.143448});
    m2::PointD userPos = {0.0, 0.0};

    EyeForTesting::RegisterMapObjectEvent(poi, MapObject::Event::Type::RouteToCreated, userPos);
  }

  {
    MapObject poi;
    poi.SetBestType("shop");
    // Sould NOT be concatenated with previous poi because of different names method returns false.
    poi.SetPos({53.652005, 108.143448});
    poi.SetDefaultName("No");
    m2::PointD userPos = {0.0, 0.0};

    EyeForTesting::RegisterMapObjectEvent(poi, MapObject::Event::Type::RouteToCreated, userPos);
  }

  {
    MapObject poi;
    poi.SetBestType("shop");
    // Sould be concatenated with previous poi because of AlmostEquals method returns true.
    poi.SetPos({53.65201, 108.1434399999});
    m2::PointD userPos = {158.016345, 53.683329};

    EyeForTesting::RegisterMapObjectEvent(poi, MapObject::Event::Type::AddToBookmark, userPos);
  }

  {
    MapObject poi;
    poi.SetBestType("shop");
    // Sould NOT be concatenated with previous poi because of AlmostEquals method returns false.
    poi.SetPos({53.65202, 108.143448});
    m2::PointD userPos = {0.0, 0.0};

    EyeForTesting::RegisterMapObjectEvent(poi, MapObject::Event::Type::UgcEditorOpened, userPos);
  }

  {
    MapObject poi;
    poi.SetBestType("amenity-bench");
    poi.SetPos({53.652005, 108.143448});
    m2::PointD userPos = {0.0, 0.0};

    EyeForTesting::RegisterMapObjectEvent(poi, MapObject::Event::Type::Open, userPos);
  }

  {
    auto const resultInfo = Eye::Instance().GetInfo();
    auto const & mapObjects = resultInfo->m_mapObjects;
    TEST_EQUAL(mapObjects.GetSize(), 5, ());

    {
      MapObject poi;
      poi.SetBestType("cafe");
      poi.SetPos({53.652005, 108.143448});

      bool found = false;
      mapObjects.ForEachInRect(poi.GetLimitRect(), [&poi, &found](MapObject const & item)
      {
        if (poi != item)
          return;

        if (!found)
          found = true;

        TEST_EQUAL(item.GetEvents().size(), 2, ());
        TEST_EQUAL(item.GetEvents()[0].m_userPos, m2::PointD(53.016347, 158.683327), ());
        TEST_EQUAL(item.GetEvents()[0].m_type, MapObject::Event::Type::Open, ());

        TEST_EQUAL(item.GetEvents()[1].m_userPos, m2::PointD(53.016345, 158.683329), ());
        TEST_EQUAL(item.GetEvents()[1].m_type, MapObject::Event::Type::RouteToCreated, ());
      });

      TEST(found, ());
    }

    {
      MapObject poi;
      poi.SetBestType("shop");
      poi.SetPos({53.652005, 108.143448});

      bool found = false;
      mapObjects.ForEachInRect(poi.GetLimitRect(), [&poi, &found](MapObject const & item)
      {
        if (poi != item)
          return;

        if (!found)
          found = true;

        TEST_EQUAL(item.GetEvents().size(), 2, ());
        TEST_EQUAL(item.GetEvents()[0].m_userPos, m2::PointD(0.0, 0.0), ());
        TEST_EQUAL(item.GetEvents()[0].m_type, MapObject::Event::Type::RouteToCreated, ());

        TEST_EQUAL(item.GetEvents()[1].m_userPos, m2::PointD(158.016345, 53.683329), ());
        TEST_EQUAL(item.GetEvents()[1].m_type, MapObject::Event::Type::AddToBookmark, ());
      });

      TEST(found, ());
    }

    {
      MapObject poi;
      poi.SetBestType("amenity-bench");
      poi.SetPos({53.652005, 108.143448});

      bool found = false;
      mapObjects.ForEachInRect(poi.GetLimitRect(), [&poi, &found](MapObject const & item)
      {
        if (poi != item)
          return;

        if (!found)
          found = true;

        TEST_EQUAL(item.GetEvents().size(), 1, ());
        TEST_EQUAL(item.GetEvents()[0].m_userPos, m2::PointD(0.0, 0.0), ());
        TEST_EQUAL(item.GetEvents()[0].m_type, MapObject::Event::Type::Open, ());
      });

      TEST(found, ());
    }
  }
}
