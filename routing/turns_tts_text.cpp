#include "routing/turns_sound_settings.hpp"
#include "routing/turns_tts_text.hpp"

#include "std/utility.hpp"


namespace routing
{
namespace turns
{
namespace sound
{
GetTtsText::GetTtsText()
{
  m_getEng.reset(new platform::GetTextById(platform::TextSource::TtsSound, "en"));
  ASSERT(m_getEng && m_getEng->IsValid(), ());
}

void GetTtsText::SetLocale(string const & locale)
{
  m_getCurLang.reset(new platform::GetTextById(platform::TextSource::TtsSound, locale));
  ASSERT(m_getCurLang && m_getCurLang->IsValid(), ());
}

string GetTtsText::operator()(Notification const & notification) const
{
  if (notification.m_distanceUnits == 0)
    return GetTextById(GetDirectionTextId(notification));

  return GetTextById(GetDistanceTextId(notification)) + " " +
      GetTextById(GetDirectionTextId(notification));
}

string GetTtsText::GetTextById(string const & textId) const
{
  ASSERT(!textId.empty(), ());

  platform::GetTextById const & getCurLang = *m_getCurLang;
  platform::GetTextById const & getEng = *m_getEng;
  if (m_getCurLang && m_getCurLang->IsValid())
  {
    pair<string, bool> const text = getCurLang(textId);
    if (text.second)
      return text.first; // textId is found in m_getCurLang
    return getEng(textId).first; // textId is not found in m_getCurLang
  }
  return getEng(textId).first;
}

string GetDistanceTextId(Notification const & notification)
{
  if (!notification.IsValid())
  {
    ASSERT(false, ());
    return "";
  }

  if (notification.m_useThenInsteadOfDistance)
    return "then";

  switch(notification.m_lengthUnits)
  {
  case LengthUnits::Undefined:
    ASSERT(false, ());
    return "";
  case LengthUnits::Meters:
    for (uint32_t const dist : soundedDistancesMeters)
    {
      if (notification.m_distanceUnits == dist)
      {
        switch(static_cast<AllSoundedDistancesMeters>(notification.m_distanceUnits))
        {
        case AllSoundedDistancesMeters::In50:
          return "in_50_meters";
        case AllSoundedDistancesMeters::In100:
          return "in_100_meters";
        case AllSoundedDistancesMeters::In200:
          return "in_200_meters";
        case AllSoundedDistancesMeters::In250:
          return "in_250_meters";
        case AllSoundedDistancesMeters::In300:
          return "in_300_meters";
        case AllSoundedDistancesMeters::In400:
          return "in_400_meters";
        case AllSoundedDistancesMeters::In500:
          return "in_500_meters";
        case AllSoundedDistancesMeters::In600:
          return "in_600_meters";
        case AllSoundedDistancesMeters::In700:
          return "in_700_meters";
        case AllSoundedDistancesMeters::In750:
          return "in_750_meters";
        case AllSoundedDistancesMeters::In800:
          return "in_800_meters";
        case AllSoundedDistancesMeters::In900:
          return "in_900_meters";
        case AllSoundedDistancesMeters::InOneKm:
          return "in_1_kilometer";
        case AllSoundedDistancesMeters::InOneAndHalfKm:
          return "in_1_5_kilometers";
        case AllSoundedDistancesMeters::InTwoKm:
          return "in_2_kilometers";
        case AllSoundedDistancesMeters::InTwoAndHalfKm:
          return "in_2_5_kilometers";
        case AllSoundedDistancesMeters::InThreeKm:
          return "in_3_kilometers";
        }
      }
    }
    ASSERT(false, ("notification.m_distanceUnits is not correct. Check soundedDistancesMeters."));
    return "";
  case LengthUnits::Feet:
    for (uint32_t const dist : soundedDistancesFeet)
    {
      if (notification.m_distanceUnits == dist)
      {
        switch(static_cast<AllSoundedDistancesFeet>(notification.m_distanceUnits))
        {
        case AllSoundedDistancesFeet::In50:
          return "in_50_feet";
        case AllSoundedDistancesFeet::In100:
          return "in_100_feet";
        case AllSoundedDistancesFeet::In200:
          return "in_200_feet";
        case AllSoundedDistancesFeet::In300:
          return "in_300_feet";
        case AllSoundedDistancesFeet::In400:
          return "in_400_feet";
        case AllSoundedDistancesFeet::In500:
          return "in_500_feet";
        case AllSoundedDistancesFeet::In600:
          return "in_600_feet";
        case AllSoundedDistancesFeet::In700:
          return "in_700_feet";
        case AllSoundedDistancesFeet::In800:
          return "in_800_feet";
        case AllSoundedDistancesFeet::In900:
          return "in_900_feet";
        case AllSoundedDistancesFeet::In1000:
          return "in_1000_feet";
        case AllSoundedDistancesFeet::In1500:
          return "in_1500_feet";
        case AllSoundedDistancesFeet::In2000:
          return "in_2000_feet";
        case AllSoundedDistancesFeet::In2500:
          return "in_2500_feet";
        case AllSoundedDistancesFeet::In3000:
          return "in_3000_feet";
        case AllSoundedDistancesFeet::In3500:
          return "in_3500_feet";
        case AllSoundedDistancesFeet::In4000:
          return "in_4000_feet";
        case AllSoundedDistancesFeet::In4500:
          return "in_4500_feet";
        case AllSoundedDistancesFeet::In5000:
          return "in_5000_feet";
        case AllSoundedDistancesFeet::InOneMile:
          return "in_1_mile";
        case AllSoundedDistancesFeet::InOneAndHalfMiles:
          return "in_1_5_miles";
        case AllSoundedDistancesFeet::InTwoMiles:
          return "in_2_miles";
        }
      }
    }
    ASSERT(false, ("notification.m_distanceUnits is not correct. Check soundedDistancesFeet."));
    return "";
  }
  ASSERT(false, ());
  return "";
}

string GetDirectionTextId(Notification const & notification)
{
  switch(notification.m_turnDir)
  {
  case TurnDirection::GoStraight:
    return "go_straight";
  case TurnDirection::TurnRight:
    return "make_a_right_turn";
  case TurnDirection::TurnSharpRight:
    return "make_a_sharp_right_turn";
  case TurnDirection::TurnSlightRight:
    return "make_a_slight_right_turn";
  case TurnDirection::TurnLeft:
    return "make_a_left_turn";
  case TurnDirection::TurnSharpLeft:
    return "make_a_sharp_left_turn";
  case TurnDirection::TurnSlightLeft:
    return "make_a_slight_left_turn";
  case TurnDirection::UTurn:
    return "make_a_u_turn";
  case TurnDirection::EnterRoundAbout:
    return "enter_the_roundabout";
  case TurnDirection::LeaveRoundAbout:
    return "leave_the_roundabout";
  case TurnDirection::ReachedYourDestination:
    return "you_have_reached_the_destination";
  case TurnDirection::StayOnRoundAbout:
  case TurnDirection::StartAtEndOfStreet:
  case TurnDirection::TakeTheExit:
  case TurnDirection::NoTurn:
  case TurnDirection::Count:
    ASSERT(false, ());
    return "";
  }
  ASSERT(false, ());
  return "";
}
} //  namespace sound
} //  namespace turns
} //  namespace routing
