#include "testing/testing.hpp"

#include "eye/eye.hpp"
#include "eye/eye_info.hpp"
#include "eye/eye_serdes.hpp"
#include "eye/eye_storage.hpp"

#include "eye/eye_tests_support/eye_for_testing.hpp"

using namespace eye;

namespace
{
Info MakeDefaultInfoForTesting()
{
  Info info;
  info.m_tips.m_lastShown = Clock::now();
  ++(info.m_tips.m_totalShownTipsCount);

  Tips::Info tipInfo;
  tipInfo.m_type = Tips::Type::DiscoverButton;
  tipInfo.m_eventCounters.Increment(Tips::Event::GotitClicked);
  tipInfo.m_lastShown = Clock::now();
  info.m_tips.m_shownTips.emplace_back(std::move(tipInfo));

  return info;
}

void CompareWithDefaultInfo(Info const & lhs, Info const & rhs)
{
  TEST_EQUAL(lhs.m_tips.m_lastShown, rhs.m_tips.m_lastShown, ());
  TEST_EQUAL(lhs.m_tips.m_totalShownTipsCount, rhs.m_tips.m_totalShownTipsCount, ());
  TEST_EQUAL(lhs.m_tips.m_shownTips.size(), rhs.m_tips.m_shownTips.size(), ());
  TEST(lhs.m_tips.m_shownTips[0].m_type == rhs.m_tips.m_shownTips[0].m_type, ());
  TEST_EQUAL(lhs.m_tips.m_shownTips[0].m_lastShown, rhs.m_tips.m_shownTips[0].m_lastShown, ());
  TEST_EQUAL(lhs.m_tips.m_shownTips[0].m_eventCounters.Get(Tips::Event::GotitClicked),
             rhs.m_tips.m_shownTips[0].m_eventCounters.Get(Tips::Event::GotitClicked), ());
}
}  // namespace

UNIT_TEST(Eye_SerdesTest)
{
  auto const info = MakeDefaultInfoForTesting();

  std::vector<int8_t> s;
  eye::Serdes::Serialize(info, s);
  Info d;
  eye::Serdes::Deserialize(s, d);

  CompareWithDefaultInfo(info, d);
}

UNIT_CLASS_TEST(ScopedEyeForTesting, SaveLoadTest)
{
  auto const info = MakeDefaultInfoForTesting();
  std::string const kEyeFileName = "eye";

  std::vector<int8_t> s;
  eye::Serdes::Serialize(info, s);
  eye::Storage::Save(eye::Storage::GetEyeFilePath(), s);
  s.clear();
  eye::Storage::Load(eye::Storage::GetEyeFilePath(), s);
  Info d;
  eye::Serdes::Deserialize(s, d);

  CompareWithDefaultInfo(info, d);
}

UNIT_CLASS_TEST(ScopedEyeForTesting, AppendTipTest)
{
  {
    auto const initialInfo = Eye::Instance().GetInfo();
    auto const & initialTips = initialInfo->m_tips;

    TEST(initialTips.m_shownTips.empty(), ());
    TEST_EQUAL(initialTips.m_totalShownTipsCount, 0, ());
    TEST_EQUAL(initialTips.m_lastShown.time_since_epoch().count(), 0, ());
  }
  {
    EyeForTesting::AppendTip(Tips::Type::DiscoverButton, Tips::Event::GotitClicked);

    auto const info = Eye::Instance().GetInfo();
    auto const & tips = info->m_tips;

    TEST_EQUAL(tips.m_shownTips.size(), 1, ());
    TEST_NOT_EQUAL(tips.m_shownTips[0].m_lastShown.time_since_epoch().count(), 0, ());
    TEST_EQUAL(tips.m_shownTips[0].m_type, Tips::Type::DiscoverButton, ());
    TEST_EQUAL(tips.m_shownTips[0].m_eventCounters.Get(Tips::Event::GotitClicked), 1, ());
    TEST_EQUAL(tips.m_shownTips[0].m_eventCounters.Get(Tips::Event::ActionClicked), 0, ());
    TEST_EQUAL(tips.m_totalShownTipsCount, 1, ());
    TEST_NOT_EQUAL(tips.m_lastShown.time_since_epoch().count(), 0, ());
    TEST_EQUAL(tips.m_shownTips[0].m_lastShown, tips.m_lastShown, ());
  }

  Time prevShowTime;
  {
    EyeForTesting::AppendTip(Tips::Type::MapsLayers, Tips::Event::ActionClicked);

    auto const info = Eye::Instance().GetInfo();
    auto const & tips = info->m_tips;

    TEST_EQUAL(tips.m_shownTips.size(), 2, ());
    TEST_NOT_EQUAL(tips.m_shownTips[1].m_lastShown.time_since_epoch().count(), 0, ());
    TEST_EQUAL(tips.m_shownTips[1].m_type, Tips::Type::MapsLayers, ());
    TEST_EQUAL(tips.m_shownTips[1].m_eventCounters.Get(Tips::Event::GotitClicked), 0, ());
    TEST_EQUAL(tips.m_shownTips[1].m_eventCounters.Get(Tips::Event::ActionClicked), 1, ());
    TEST_EQUAL(tips.m_totalShownTipsCount, 2, ());
    TEST_NOT_EQUAL(tips.m_lastShown.time_since_epoch().count(), 0, ());
    TEST_EQUAL(tips.m_shownTips[1].m_lastShown, tips.m_lastShown, ());

    prevShowTime = tips.m_lastShown;
  }
  {
    EyeForTesting::AppendTip(Tips::Type::MapsLayers, Tips::Event::GotitClicked);

    auto const info = Eye::Instance().GetInfo();
    auto const & tips = info->m_tips;

    TEST_EQUAL(tips.m_shownTips.size(), 2, ());
    TEST_NOT_EQUAL(tips.m_shownTips[1].m_lastShown.time_since_epoch().count(), 0, ());
    TEST_EQUAL(tips.m_shownTips[1].m_type, Tips::Type::MapsLayers, ());
    TEST_EQUAL(tips.m_shownTips[1].m_eventCounters.Get(Tips::Event::GotitClicked), 1, ());
    TEST_EQUAL(tips.m_shownTips[1].m_eventCounters.Get(Tips::Event::ActionClicked), 1, ());
    TEST_EQUAL(tips.m_totalShownTipsCount, 3, ());
    TEST_NOT_EQUAL(tips.m_lastShown.time_since_epoch().count(), 0, ());
    TEST_EQUAL(tips.m_shownTips[1].m_lastShown, tips.m_lastShown, ());
    TEST_NOT_EQUAL(prevShowTime, tips.m_lastShown, ());
  }
}
