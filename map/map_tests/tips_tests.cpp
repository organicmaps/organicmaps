#include "testing/testing.hpp"

#include "map/tips_api.hpp"

#include "eye/eye_tests_support/eye_for_testing.hpp"

#include "eye/eye.hpp"

#include <algorithm>

using namespace eye;

namespace
{
void MakeLastShownTipAvailableByTime()
{
  auto const info = Eye::Instance().GetInfo();
  auto editableInfo = *info;
  auto & tips = editableInfo.m_tips;
  tips.m_lastShown = Time(TipsApi::GetShowAnyTipPeriod() + std::chrono::seconds(1));
  tips.m_shownTips.back().m_lastShown =
      Time(TipsApi::GetShowSameTipPeriod() + std::chrono::seconds(1));
  EyeForTesting::SetInfo(editableInfo);
}

TipsApi::Tip GetTipForTesting(TipsApi::Duration showAnyTipPeriod,
                              TipsApi::Duration showSameTipPeriod)
{
  TipsApi::Conditions conditions = {
      {[] { return true; }, [] { return true; }, [] { return true; }, [] { return true; }}};
  return TipsApi::GetTipForTesting(showAnyTipPeriod, showSameTipPeriod, conditions);
}

TipsApi::Tip GetTipForTesting()
{
  return GetTipForTesting(TipsApi::GetShowAnyTipPeriod(), TipsApi::GetShowSameTipPeriod());
}

void ShowTipWithClickCountTest(Tips::Event eventType, size_t maxClickCount)
{
  std::vector<Tips::Type> usedTips;
  auto previousTip = GetTipForTesting();

  TEST(previousTip.is_initialized(), ());
  TEST_NOT_EQUAL(previousTip.get(), Tips::Type::Count, ());

  auto const totalTipsCount = static_cast<size_t>(Tips::Type::Count);

  for (size_t i = 0; i < totalTipsCount; ++i)
  {
    auto tip = GetTipForTesting();

    TEST(tip.is_initialized(), ());
    TEST_NOT_EQUAL(tip.get(), Tips::Type::Count, ());

    EyeForTesting::AppendTip(tip.get(), eventType);

    boost::optional<Tips::Type> secondTip;
    for (size_t j = 1; j < maxClickCount; ++j)
    {
      MakeLastShownTipAvailableByTime();

      secondTip = GetTipForTesting();
      TEST(secondTip.is_initialized(), ());

      EyeForTesting::AppendTip(tip.get(), eventType);
    }

    MakeLastShownTipAvailableByTime();

    secondTip = GetTipForTesting();
    TEST(!secondTip.is_initialized() || secondTip.get() != tip.get(), ());
  }

  auto emptyTip = GetTipForTesting();
  TEST(!emptyTip.is_initialized(), ());
}

UNIT_CLASS_TEST(ScopedEyeForTesting, ShowAnyTipPeriod_Test)
{
  auto firstTip = GetTipForTesting();

  TEST(firstTip.is_initialized(), ());
  TEST_NOT_EQUAL(firstTip.get(), Tips::Type::Count, ());

  EyeForTesting::AppendTip(firstTip.get(), Tips::Event::GotitClicked);

  auto secondTip = GetTipForTesting();

  TEST(!secondTip.is_initialized(), ());
}

UNIT_CLASS_TEST(ScopedEyeForTesting, ShowFirstTip_Test)
{
  std::vector<Tips::Type> usedTips;
  auto previousTip = GetTipForTesting();

  TEST(previousTip.is_initialized(), ());
  TEST_NOT_EQUAL(previousTip.get(), Tips::Type::Count, ());

  auto const totalTipsCount = static_cast<size_t>(Tips::Type::Count);

  for (size_t i = 0; i < totalTipsCount; ++i)
  {
    auto tip = GetTipForTesting({}, TipsApi::GetShowSameTipPeriod());

    TEST(tip.is_initialized(), ());
    TEST_NOT_EQUAL(tip.get(), Tips::Type::Count, ());

    auto const it = std::find(usedTips.cbegin(), usedTips.cend(), tip.get());
    TEST(it == usedTips.cend(), ());

    if (i % 2 == 0)
      EyeForTesting::AppendTip(tip.get(), Tips::Event::ActionClicked);
    else
      EyeForTesting::AppendTip(tip.get(), Tips::Event::GotitClicked);

    TEST(!GetTipForTesting().is_initialized(), ());

    usedTips.emplace_back(tip.get());
  }

  auto emptyTip = GetTipForTesting();
  TEST(!emptyTip.is_initialized(), ());
}

UNIT_CLASS_TEST(ScopedEyeForTesting, ShowTipAndActionClicked_Test)
{
  ShowTipWithClickCountTest(Tips::Event::ActionClicked, TipsApi::GetActionClicksCountToDisable());
}

UNIT_CLASS_TEST(ScopedEyeForTesting, ShowTipAndGotitClicked_Test)
{
  ShowTipWithClickCountTest(Tips::Event::GotitClicked, TipsApi::GetGotitClicksCountToDisable());
}
}  // namespace
