#include "testing/testing.hpp"

#include "map/tips_api.hpp"

#include "metrics/metrics_tests_support/eye_for_testing.hpp"

#include "metrics/eye.hpp"
#include "metrics/eye_info.hpp"

#include "base/timer.hpp"

#include <algorithm>
#include <vector>

#include <boost/optional.hpp>

using namespace eye;

namespace
{
class TipsApiDelegate : public TipsApi::Delegate
{
public:
  void SetLastBackgroundTime(double lastBackgroundTime)
  {
    m_lastBackgroundTime = lastBackgroundTime;
  }

  // TipsApi::Delegate overrides:
  boost::optional<m2::PointD> GetCurrentPosition() const override
  {
    return {};
  }

  bool IsCountryLoaded(m2::PointD const & pt) const override { return false; }

  bool HaveTransit(m2::PointD const & pt) const override { return false; }

  double GetLastBackgroundTime() const override { return m_lastBackgroundTime; }

private:
  double m_lastBackgroundTime = 0.0;
};

void MakeLastShownTipAvailableByTime()
{
  auto const info = Eye::Instance().GetInfo();
  auto editableInfo = *info;
  auto & tips = editableInfo.m_tips;
  tips.back().m_lastShownTime =
      Time(TipsApi::GetShowSameTipPeriod() + std::chrono::seconds(1));
  EyeForTesting::SetInfo(editableInfo);
}

boost::optional<eye::Tip::Type> GetTipForTesting(TipsApi::Duration showAnyTipPeriod,
                                                 TipsApi::Duration showSameTipPeriod,
                                                 TipsApiDelegate const & delegate)
{
  // Do not use additional conditions for testing.
  TipsApi::Conditions conditions =
  {{
     // Condition for Tips::Type::BookmarksCatalog type.
    [] (eye::Info const & info) { return true; },
    // Condition for Tips::Type::BookingHotels type.
    [] (eye::Info const & info) { return true; },
    // Condition for Tips::Type::DiscoverButton type.
    [] (eye::Info const & info) { return true; },
     // Condition for Tips::Type::PublicTransport type.
    [] (eye::Info const & info) { return true; }
  }};
  return TipsApi::GetTipForTesting(showAnyTipPeriod, showSameTipPeriod, delegate, conditions);
}

boost::optional<eye::Tip::Type> GetTipForTesting()
{
  return GetTipForTesting(TipsApi::GetShowAnyTipPeriod(), TipsApi::GetShowSameTipPeriod(), {});
}

void ShowTipWithClickCountTest(Tip::Event eventType, size_t maxClickCount)
{
  std::vector<Tip::Type> usedTips;
  auto previousTip = GetTipForTesting();

  TEST(previousTip.is_initialized(), ());
  TEST_NOT_EQUAL(previousTip.get(), Tip::Type::Count, ());

  auto const totalTipsCount = static_cast<size_t>(Tip::Type::Count);

  for (size_t i = 0; i < totalTipsCount; ++i)
  {
    auto tip = GetTipForTesting();

    TEST(tip.is_initialized(), ());
    TEST_NOT_EQUAL(tip.get(), Tip::Type::Count, ());

    EyeForTesting::AppendTip(tip.get(), eventType);

    boost::optional<Tip::Type> secondTip;
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
  TEST_NOT_EQUAL(firstTip.get(), Tip::Type::Count, ());

  EyeForTesting::AppendTip(firstTip.get(), Tip::Event::GotitClicked);

  auto secondTip = GetTipForTesting();

  TEST(!secondTip.is_initialized(), ());
}

UNIT_CLASS_TEST(ScopedEyeForTesting, ShowFirstTip_Test)
{
  std::vector<Tip::Type> usedTips;
  auto previousTip = GetTipForTesting();

  TEST(previousTip.is_initialized(), ());
  TEST_NOT_EQUAL(previousTip.get(), Tip::Type::Count, ());

  auto const totalTipsCount = static_cast<size_t>(Tip::Type::Count);

  for (size_t i = 0; i < totalTipsCount; ++i)
  {
    auto tip = GetTipForTesting({}, TipsApi::GetShowSameTipPeriod(), {});

    TEST(tip.is_initialized(), ());
    TEST_NOT_EQUAL(tip.get(), Tip::Type::Count, ());

    auto const it = std::find(usedTips.cbegin(), usedTips.cend(), tip.get());
    TEST(it == usedTips.cend(), ());

    if (i % 2 == 0)
      EyeForTesting::AppendTip(tip.get(), Tip::Event::ActionClicked);
    else
      EyeForTesting::AppendTip(tip.get(), Tip::Event::GotitClicked);

    TEST(!GetTipForTesting().is_initialized(), ());

    usedTips.emplace_back(tip.get());
  }

  auto emptyTip = GetTipForTesting();
  TEST(!emptyTip.is_initialized(), ());
}

UNIT_CLASS_TEST(ScopedEyeForTesting, ShowTipAndActionClicked_Test)
{
  ShowTipWithClickCountTest(Tip::Event::ActionClicked, TipsApi::GetActionClicksCountToDisable());
}

UNIT_CLASS_TEST(ScopedEyeForTesting, ShowTipAndGotitClicked_Test)
{
  ShowTipWithClickCountTest(Tip::Event::GotitClicked, TipsApi::GetGotitClicksCountToDisable());
}

UNIT_CLASS_TEST(ScopedEyeForTesting, ShowTipAfterWarmStart)
{
  TipsApiDelegate d;
  d.SetLastBackgroundTime(my::Timer::LocalTime());
  auto tip = GetTipForTesting({}, TipsApi::GetShowSameTipPeriod(), d);
  TEST(!tip.is_initialized(), ());
  d.SetLastBackgroundTime(my::Timer::LocalTime() - TipsApi::ShowTipAfterCollapsingPeriod().count());
  tip = GetTipForTesting({}, TipsApi::GetShowSameTipPeriod(), d);
  TEST(tip.is_initialized(), ());
}
}  // namespace
