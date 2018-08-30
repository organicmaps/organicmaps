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
    EyeForTesting::AppendTip(Tip::Type::MapsLayers, Tip::Event::ActionClicked);

    auto const info = Eye::Instance().GetInfo();
    auto const & tips = info->m_tips;
    auto const lastShownTipTime = GetLastShownTipTime(tips);

    TEST_EQUAL(tips.size(), 2, ());
    TEST_NOT_EQUAL(tips[1].m_lastShownTime.time_since_epoch().count(), 0, ());
    TEST_EQUAL(tips[1].m_type, Tip::Type::MapsLayers, ());
    TEST_EQUAL(tips[1].m_eventCounters.Get(Tip::Event::GotitClicked), 0, ());
    TEST_EQUAL(tips[1].m_eventCounters.Get(Tip::Event::ActionClicked), 1, ());
    TEST_NOT_EQUAL(lastShownTipTime.time_since_epoch().count(), 0, ());
    TEST_EQUAL(tips[1].m_lastShownTime, lastShownTipTime, ());

    prevShowTime = lastShownTipTime;
  }
  {
    EyeForTesting::AppendTip(Tip::Type::MapsLayers, Tip::Event::GotitClicked);

    auto const info = Eye::Instance().GetInfo();
    auto const & tips = info->m_tips;
    auto const lastShownTipTime = GetLastShownTipTime(tips);

    TEST_EQUAL(tips.size(), 2, ());
    TEST_NOT_EQUAL(tips[1].m_lastShownTime.time_since_epoch().count(), 0, ());
    TEST_EQUAL(tips[1].m_type, Tip::Type::MapsLayers, ());
    TEST_EQUAL(tips[1].m_eventCounters.Get(Tip::Event::GotitClicked), 1, ());
    TEST_EQUAL(tips[1].m_eventCounters.Get(Tip::Event::ActionClicked), 1, ());
    TEST_NOT_EQUAL(lastShownTipTime.time_since_epoch().count(), 0, ());
    TEST_EQUAL(tips[1].m_lastShownTime, lastShownTipTime, ());
    TEST_NOT_EQUAL(prevShowTime, lastShownTipTime, ());
  }
}
