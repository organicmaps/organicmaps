#include "map/tips_api.hpp"

#include "metrics/eye.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"
#include "base/timer.hpp"

#include <type_traits>
#include <unordered_set>
#include <utility>

using namespace eye;

namespace
{
// The app shouldn't show any screen at all more frequently than once in 3 days.
auto constexpr kShowAnyTipPeriod = std::chrono::hours(24) * 3;
// The app shouldn't show the same screen more frequently than 1 month.
auto constexpr kShowSameTipPeriod = std::chrono::hours(24) * 30;
// Every current trigger for a tips screen can be activated at the start of the application by
// default and if the app was inactive more then 12 hours.
auto constexpr kShowTipAfterCollapsingPeriod = std::chrono::hours(12);
// If a user clicks on the action areas (highlighted or blue button)
// the appropriate screen will never be shown again.
size_t constexpr kActionClicksCountToDisable = 1;
// If a user clicks 3 times on the button GOT IT the appropriate screen will never be shown again.
size_t constexpr kGotitClicksCountToDisable = 3;

std::unordered_set<std::string> const kIsolinesExceptedMwms =
{
  "Argentina_Buenos Aires_Buenos Aires",
  "Australia_Melbourne",
  "Australia_Sydney",
  "Austria_Salzburg",
  "Belarus_Minsk Region",
  "China_Guizhou",
  "China_Shanghai",
  "Czech_Praha",
  "Finland_Southern Finland_Helsinki",
  "France_Ile-de-France_Paris",
  "Germany_Berlin",
  "Germany_Hamburg_main",
  "Germany_Saxony_Leipzig",
  "Germany_Saxony_Dresden",
  "India_Delhi",
  "Italy_Veneto_Venezia",
  "Italy_Veneto_Verona",
  "Netherlands_Utrecht_Utrecht",
  "Netherlands_North Holland_Amsterdam",
  "Russia_Moscow",
  "Spain_Catalonia_Provincia de Barcelona",
  "Spain_Community of Madrid",
  "UK_England_Greater London",
  "US_New York_New York",
  "Russia_Saint Petersburg",
  "US_Illinois_Chickago"
};

template <typename T, std::enable_if_t<std::is_enum<T>::value> * = nullptr>
size_t ToIndex(T type)
{
  return static_cast<size_t>(type);
}

std::optional<eye::Tip::Type> GetTipImpl(TipsApi::Duration showAnyTipPeriod,
                                         TipsApi::Duration showSameTipPeriod,
                                         TipsApi::Delegate const & delegate,
                                         TipsApi::Conditions const & conditions)
{
  auto const lastBackgroundTime = delegate.GetLastBackgroundTime();
  if (lastBackgroundTime != 0.0)
  {
    auto const timeInBackground =
        static_cast<uint64_t>(base::Timer::LocalTime() - lastBackgroundTime);
    if (timeInBackground < kShowTipAfterCollapsingPeriod.count())
      return {};
  }

  auto const info = Eye::Instance().GetInfo();

  CHECK(info, ("Eye info must be initialized"));

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

    // Iterates reversed because we need to show newest tip first.
    for (auto c = candidates.crbegin(); c != candidates.crend(); ++c)
    {
      if (c->second && conditions[ToIndex(c->first)](*info))
        return c->first;
    }
  }

  for (auto const & shownTip : tips)
  {
    if (shownTip.m_eventCounters.Get(Tip::Event::ActionClicked) < kActionClicksCountToDisable &&
        shownTip.m_eventCounters.Get(Tip::Event::GotitClicked) < kGotitClicksCountToDisable &&
        Clock::now() - shownTip.m_lastShownTime > showSameTipPeriod &&
        conditions[ToIndex(shownTip.m_type)](*info))
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
TipsApi::Duration TipsApi::ShowTipAfterCollapsingPeriod()
{
  return kShowTipAfterCollapsingPeriod;
};

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

TipsApi::TipsApi(std::unique_ptr<Delegate> delegate)
  : m_delegate(std::move(delegate))
{
  m_conditions =
  {{
    // Condition for Tips::Type::BookmarksCatalog type.
    [] (eye::Info const & info)
    {
      return info.m_bookmarks.m_lastOpenedTime.time_since_epoch().count() == 0 &&
        GetPlatform().ConnectionStatus() != Platform::EConnectionType::CONNECTION_NONE;
    },
    // Condition for Tips::Type::BookingHotels type.
    [] (eye::Info const & info)
    {
      return info.m_booking.m_lastFilterUsedTime.time_since_epoch().count() == 0;
    },
    // Condition for Tips::Type::DiscoverButton type.
    [this] (eye::Info const & info)
    {
      auto const eventsCount = ToIndex(Discovery::Event::Count);
      for (size_t i = 0; i < eventsCount; ++i)
      {
        if (info.m_discovery.m_eventCounters.Get(static_cast<Discovery::Event>(i)) != 0)
          return false;
      }

      auto const pos = m_delegate->GetCurrentPosition();
      if (!pos)
        return false;

      return m_delegate->IsCountryLoaded(*pos);
    },
    // Condition for Tips::Type::PublicTransport type.
    [this] (eye::Info const & info)
    {
      for (auto const & layer : info.m_layers)
      {
        if (layer.m_type == Layer::Type::PublicTransport &&
            layer.m_lastTimeUsed.time_since_epoch().count() != 0)
        {
          return false;
        }
      }

      auto const pos = m_delegate->GetCurrentPosition();
      if (!pos)
        return false;

      return m_delegate->HaveTransit(*pos);
    },
   // Condition for Tips::Type::Isolines type.
   [this] (eye::Info const & info)
   {
     for (auto const & layer : info.m_layers)
     {
       if (layer.m_type == Layer::Type::Isolines &&
           layer.m_lastTimeUsed.time_since_epoch().count() != 0)
       {
         return false;
       }
     }

     auto const pos = m_delegate->GetViewportCenter();
     auto const countryId = m_delegate->GetCountryId(pos);

     if (countryId.empty())
       return false;

     if (kIsolinesExceptedMwms.find(countryId) != kIsolinesExceptedMwms.end())
         return false;

     return m_delegate->GetIsolinesQuality(countryId) == isolines::Quality::Normal;
   },
  }};
}

std::optional<eye::Tip::Type> TipsApi::GetTip() const
{
  if (!m_isEnabled)
    return {};

  return GetTipImpl(GetShowAnyTipPeriod(), GetShowSameTipPeriod(), *m_delegate, m_conditions);
}

void TipsApi::SetEnabled(bool isEnabled)
{
  m_isEnabled = isEnabled;
}

// static
std::optional<eye::Tip::Type> TipsApi::GetTipForTesting(Duration showAnyTipPeriod,
                                                        Duration showSameTipPeriod,
                                                        TipsApi::Delegate const & delegate,
                                                        Conditions const & triggers)
{
  return GetTipImpl(showAnyTipPeriod, showSameTipPeriod, delegate, triggers);
}
