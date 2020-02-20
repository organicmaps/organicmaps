#include "testing/testing.hpp"

#include "map/tips_api.hpp"

#include "metrics/metrics_tests_support/eye_for_testing.hpp"

#include "metrics/eye.hpp"
#include "metrics/eye_info.hpp"

#include "base/timer.hpp"

#include <algorithm>
#include <optional>
#include <vector>

using namespace eye;

namespace
{
class TipsApiDelegateForTesting : public TipsApi::Delegate
{
public:
  void SetLastBackgroundTime(double lastBackgroundTime)
  {
    m_lastBackgroundTime = lastBackgroundTime;
  }

  // TipsApi::Delegate overrides:
  std::optional<m2::PointD> GetCurrentPosition() const override { return {}; }

  bool IsCountryLoaded(m2::PointD const & pt) const override { return false; }

  bool HaveTransit(m2::PointD const & pt) const override { return false; }

  double GetLastBackgroundTime() const override { return m_lastBackgroundTime; }

  m2::PointD const & GetViewportCenter() const override { return m_point; }
  storage::CountryId GetCountryId(m2::PointD const & pt) const override { return ""; }
  isolines::Quality GetIsolinesQuality(storage::CountryId const & countryId) const override
  {
    return isolines::Quality::None;
  }

private:
  double m_lastBackgroundTime = 0.0;
  m2::PointD m_point;
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

std::optional<eye::Tip::Type> GetTipForTesting(TipsApi::Duration showAnyTipPeriod,
                                                 TipsApi::Duration showSameTipPeriod,
                                                 TipsApiDelegateForTesting const & delegate)
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
    [] (eye::Info const & info) { return true; },
    // Condition for Tips::Type::Isolines type.
    [] (eye::Info const & info) { return true; }
  }};
  return TipsApi::GetTipForTesting(showAnyTipPeriod, showSameTipPeriod, delegate, conditions);
}

std::optional<eye::Tip::Type> GetTipForTesting()
{
  return GetTipForTesting(TipsApi::GetShowAnyTipPeriod(), TipsApi::GetShowSameTipPeriod(), {});
}

void ShowTipWithClickCountTest(Tip::Event eventType, size_t maxClickCount)
{
  std::vector<Tip::Type> usedTips;
  auto previousTip = GetTipForTesting();

  TEST(previousTip.has_value(), ());
  TEST_NOT_EQUAL(*previousTip, Tip::Type::Count, ());

  auto const totalTipsCount = static_cast<size_t>(Tip::Type::Count);

  for (size_t i = 0; i < totalTipsCount; ++i)
  {
    auto tip = GetTipForTesting();

    TEST(tip.has_value(), ());
    TEST_NOT_EQUAL(*tip, Tip::Type::Count, ());

    EyeForTesting::AppendTip(*tip, eventType);

    std::optional<Tip::Type> secondTip;
    for (size_t j = 1; j < maxClickCount; ++j)
    {
      MakeLastShownTipAvailableByTime();

      secondTip = GetTipForTesting();
      TEST(secondTip.has_value(), ());

      EyeForTesting::AppendTip(*tip, eventType);
    }

    MakeLastShownTipAvailableByTime();

    secondTip = GetTipForTesting();
    TEST(!secondTip.has_value() || *secondTip != *tip, ());
  }

  auto emptyTip = GetTipForTesting();
  TEST(!emptyTip.has_value(), ());
}

UNIT_CLASS_TEST(ScopedEyeForTesting, ShowAnyTipPeriod_Test)
{
  auto firstTip = GetTipForTesting();

  TEST(firstTip.has_value(), ());
  TEST_NOT_EQUAL(*firstTip, Tip::Type::Count, ());

  EyeForTesting::AppendTip(*firstTip, Tip::Event::GotitClicked);

  auto secondTip = GetTipForTesting();

  TEST(!secondTip.has_value(), ());
}

UNIT_CLASS_TEST(ScopedEyeForTesting, ShowFirstTip_Test)
{
  std::vector<Tip::Type> usedTips;
  auto previousTip = GetTipForTesting();

  TEST(previousTip.has_value(), ());
  TEST_NOT_EQUAL(*previousTip, Tip::Type::Count, ());

  auto const totalTipsCount = static_cast<size_t>(Tip::Type::Count);

  for (size_t i = 0; i < totalTipsCount; ++i)
  {
    auto tip = GetTipForTesting({}, TipsApi::GetShowSameTipPeriod(), {});

    TEST(tip.has_value(), ());
    TEST_NOT_EQUAL(*tip, Tip::Type::Count, ());

    auto const it = std::find(usedTips.cbegin(), usedTips.cend(), *tip);
    TEST(it == usedTips.cend(), ());

    if (i % 2 == 0)
      EyeForTesting::AppendTip(*tip, Tip::Event::ActionClicked);
    else
      EyeForTesting::AppendTip(*tip, Tip::Event::GotitClicked);

    TEST(!GetTipForTesting().has_value(), ());

    usedTips.emplace_back(*tip);
  }

  auto emptyTip = GetTipForTesting();
  TEST(!emptyTip.has_value(), ());
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
  TipsApiDelegateForTesting d;
  d.SetLastBackgroundTime(base::Timer::LocalTime());
  auto tip = GetTipForTesting({}, TipsApi::GetShowSameTipPeriod(), d);
  TEST(!tip.has_value(), ());
  d.SetLastBackgroundTime(base::Timer::LocalTime() - TipsApi::ShowTipAfterCollapsingPeriod().count());
  tip = GetTipForTesting({}, TipsApi::GetShowSameTipPeriod(), d);
  TEST(tip.has_value(), ());
}
}  // namespace
