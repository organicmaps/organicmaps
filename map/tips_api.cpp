#include "map/tips_api.hpp"

#include "metrics/eye.hpp"

#include "platform/platform.hpp"

#include <array>
#include <utility>

using namespace eye;

namespace
{
// The app shouldn't show any screen at all more frequently than once in 3 days.
auto const kShowAnyTipPeriod = std::chrono::hours(24) * 3;
// The app shouldn't show the same screen more frequently than 1 month.
auto const kShowSameTipPeriod = std::chrono::hours(24) * 30;
// If a user clicks on the action areas (highlighted or blue button)
// the appropriate screen will never be shown again.
size_t const kActionClicksCountToDisable = 1;
// If a user clicks 3 times on the button GOT IT the appropriate screen will never be shown again.
size_t const kGotitClicksCountToDisable = 3;

size_t ToIndex(Tips::Type type)
{
  return static_cast<size_t>(type);
}

TipsApi::Tip GetTipImpl(TipsApi::Duration showAnyTipPeriod, TipsApi::Duration showSameTipPeriod,
                        TipsApi::Conditions const & triggers)
{
  auto const info = Eye::Instance().GetInfo();
  auto const & tips = info->m_tips;
  auto const totalTipsCount = static_cast<size_t>(Tips::Type::Count);

  if (Clock::now() - tips.m_lastShown <= showAnyTipPeriod)
    return {};

  // If some tips are never shown.
  if (tips.m_shownTips.size() < totalTipsCount)
  {
    using Candidate = std::pair<Tips::Type, bool>;
    std::array<Candidate, totalTipsCount> candidates;
    for (size_t i = 0; i < totalTipsCount; ++i)
    {
      candidates[i] = {static_cast<Tips::Type>(i), true};
    }

    for (auto const & shownTip : tips.m_shownTips)
    {
      candidates[ToIndex(shownTip.m_type)].second = false;
    }

    for (auto const & c : candidates)
    {
      if (c.second && triggers[ToIndex(c.first)]())
        return c.first;
    }
  }

  for (auto const & shownTip : tips.m_shownTips)
  {
    if (shownTip.m_eventCounters.Get(Tips::Event::ActionClicked) < kActionClicksCountToDisable &&
        shownTip.m_eventCounters.Get(Tips::Event::GotitClicked) < kGotitClicksCountToDisable &&
        Clock::now() - shownTip.m_lastShown > showSameTipPeriod &&
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

TipsApi::TipsApi()
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
      auto const pos = m_delegate->GetCurrentPosition();
      if (!pos.is_initialized())
        return false;

      return m_delegate->IsCountryLoaded(pos.get());
    },
    // Condition for Tips::Type::MapsLayers type.
    [this]
    {
      auto const pos = m_delegate->GetCurrentPosition();
      if (!pos.is_initialized())
        return false;

      return m_delegate->HaveTransit(pos.get());
    }
  }};
}

void TipsApi::SetDelegate(std::unique_ptr<Delegate> delegate)
{
  m_delegate = std::move(delegate);
}

TipsApi::Tip TipsApi::GetTip() const
{
  return GetTipImpl(GetShowAnyTipPeriod(), GetShowSameTipPeriod(), m_conditions);
}

// static
TipsApi::Tip TipsApi::GetTipForTesting(Duration showAnyTipPeriod, Duration showSameTipPeriod,
                                       Conditions const & triggers)
{
  return GetTipImpl(showAnyTipPeriod, showSameTipPeriod, triggers);
}
