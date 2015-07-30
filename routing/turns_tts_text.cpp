#include "routing/turns_sound_settings.hpp"
#include "routing/turns_tts_text.hpp"

#include "std/algorithm.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"


namespace
{
  using namespace routing::turns::sound;
  string DistToTextId(VecPairDist::const_iterator begin, VecPairDist::const_iterator end, uint32_t dist)
  {
    VecPairDist::const_iterator distToSound =
        lower_bound(begin, end, dist, [](PairDist const & p1, uint32_t p2)
        {
          return p1.first < p2;
        });
    if (distToSound == end)
    {
      ASSERT(false, ("notification.m_distanceUnits is not correct."));
      return "";
    }
    return distToSound->second;
  }
} //  namespace

namespace routing
{
namespace turns
{
namespace sound
{
void GetTtsText::SetLocale(string const & locale)
{
  m_getCurLang.reset(new platform::GetTextById(platform::TextSource::TtsSound, locale));
  ASSERT(m_getCurLang && m_getCurLang->IsValid(), ());
}

void GetTtsText::SetLocaleWithJson(string const & jsonBuffer)
{
  m_getCurLang.reset(new platform::GetTextById(jsonBuffer));
  ASSERT(m_getCurLang && m_getCurLang->IsValid(), ());
}

string GetTtsText::operator()(Notification const & notification) const
{
  if (notification.m_distanceUnits == 0 && !notification.m_useThenInsteadOfDistance)
    return GetTextById(GetDirectionTextId(notification));

  return GetTextById(GetDistanceTextId(notification)) + " " +
      GetTextById(GetDirectionTextId(notification));
}

string GetTtsText::GetTextById(string const & textId) const
{
  ASSERT(!textId.empty(), ());

  if (!m_getCurLang || !m_getCurLang->IsValid())
  {
    ASSERT(false, ());
    return "";
  }
  return (*m_getCurLang)(textId);
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
    return DistToTextId(GetAllSoundedDistMeters().cbegin(), GetAllSoundedDistMeters().cend(),
                        notification.m_distanceUnits);
  case LengthUnits::Feet:
    return DistToTextId(GetAllSoundedDistFeet().cbegin(), GetAllSoundedDistFeet().cend(),
                        notification.m_distanceUnits);
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
