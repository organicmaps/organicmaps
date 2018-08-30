#include "map/tips_api.hpp"

#include "metrics/eye.hpp"

#include "platform/platform.hpp"

#include <utility>

using namespace eye;

namespace
{
// The app shouldn't show any screen at all more frequently than once in 3 days.
auto constexpr kShowAnyTipPeriod = std::chrono::hours(24) * 3;
// The app shouldn't show the same screen more frequently than 1 month.
auto constexpr kShowSameTipPeriod = std::chrono::hours(24) * 30;
// If a user clicks on the action areas (highlighted or blue button)
// the appropriate screen will never be shown again.
size_t constexpr kActionClicksCountToDisable = 1;
// If a user clicks 3 times on the button GOT IT the appropriate screen will never be shown again.
size_t constexpr kGotitClicksCountToDisable = 3;

size_t ToIndex(Tip::Type type)
{
  return static_cast<size_t>(type);
}

boost::optional<eye::Tip::Type> GetTipImpl(TipsApi::Duration showAnyTipPeriod,
                                           TipsApi::Duration showSameTipPeriod,
                                           TipsApi::Conditions const & triggers)
{
  auto const info = Eye::Instance().GetInfo();
  auto const & tips = info->m_tips;
  auto constexpr totalTipsCount = static_cast<size_t>(Tip::Type::Count);

  Time lastShownTime;
  for (auto const & tip : tips)
  {
    if (lastShownTime < tip.m_lastShownTime)
      lastShownTime = tip.m_lastShownTime;
  }

  if (Clock::now() - lastShownTime <= showAnyTipPeriod)
    return {};

  // If some tips are never shown.
  if (tips.size() < totalTipsCount)
  {
    using Candidate = std::pair<Tip::Type, bool>;
    std::array<Candidate, totalTipsCount> candidates;
    for (size_t i = 0; i < totalTipsCount; ++i)
    {
      candidates[i] = {static_cast<Tip::Type>(i), true};
    }

    for (auto const & shownTip : tips)
    {
      candidates[ToIndex(shownTip.m_type)].second = false;
    }

    for (auto const & c : candidates)
    {
      if (c.second && triggers[ToIndex(c.first)]())
        return c.first;
    }
  }

  for (auto const & shownTip : tips)
  {
    if (shownTip.m_eventCounters.Get(Tip::Event::ActionClicked) < kActionClicksCountToDisable &&
        shownTip.m_eventCounters.Get(Tip::Event::GotitClicked) < kGotitClicksCountToDisable &&
        Clock::now() - shownTip.m_lastShownTime > showSameTipPeriod &&
        triggers[ToIndex(shownTip.m_type)]())
    {
      return shownTip.m_type;
    }
  }

  return {};
}
}  // namespace

// static
TipsApi::Duration TipsApi::GetShowAnyTipPeriod()
{
  return kShowAnyTipPeriod;
}

// static
TipsApi::Duration TipsApi::GetShowSameTipPeriod()
{
  return kShowSameTipPeriod;
}

// static
size_t TipsApi::GetActionClicksCountToDisable()
{
 return kActionClicksCountToDisable;
}

// static
size_t TipsApi::GetGotitClicksCountToDisable()
{
  return kGotitClicksCountToDisable;
}

TipsApi::TipsApi(Delegate const & delegate)
  : m_delegate(delegate)
{
  m_conditions =
  {{
    // Condition for Tips::Type::BookmarksCatalog type.
    [] { return GetPlatform().ConnectionStatus() != Platform::EConnectionType::CONNECTION_NONE; },
    // Condition for Tips::Type::BookingHotels type.
    [] { return true; },
    // Condition for Tips::Type::DiscoverButton type.
    [this]
    {
      auto const pos = m_delegate.GetCurrentPosition();
      if (!pos.is_initialized())
        return false;

      return m_delegate.IsCountryLoaded(pos.get());
    },
    // Condition for Tips::Type::MapsLayers type.
    [this]
    {
      auto const pos = m_delegate.GetCurrentPosition();
      if (!pos.is_initialized())
        return false;

      return m_delegate.HaveTransit(pos.get());
    }
  }};
}

boost::optional<eye::Tip::Type> TipsApi::GetTip() const
{
  return GetTipImpl(GetShowAnyTipPeriod(), GetShowSameTipPeriod(), m_conditions);
}

// static
boost::optional<eye::Tip::Type> TipsApi::GetTipForTesting(Duration showAnyTipPeriod,
                                                          Duration showSameTipPeriod,
                                                          Conditions const & triggers)
{
  return GetTipImpl(showAnyTipPeriod, showSameTipPeriod, triggers);
}
