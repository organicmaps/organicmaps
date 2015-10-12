#include "routing/turns_sound_settings.hpp"
#include "routing/turns_tts_text.hpp"

#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"

namespace
{
using namespace routing::turns::sound;

template <class TIter> string DistToTextId(TIter begin, TIter end, uint32_t dist)
{
  using TValue = typename iterator_traits<TIter>::value_type;

  TIter distToSound = lower_bound(begin, end, dist, [](TValue const & p1, uint32_t p2)
                      {
                        return p1.first < p2;
                      });
  if (distToSound == end)
  {
    ASSERT(false, ("notification.m_distanceUnits is not correct."));
    return string();
  }
  return distToSound->second;
}
}  //  namespace

namespace routing
{
namespace turns
{
namespace sound
{
void GetTtsText::SetLocale(string const & locale)
{
  m_locale = locale;
  m_getCurLang.reset(new platform::GetTextById(platform::TextSource::TtsSound, locale));
  /// @todo Factor out file check from constructor and do not create m_getCurLang object in case of error.
  ASSERT(m_getCurLang, ());
}

void GetTtsText::ForTestingSetLocaleWithJson(string const & jsonBuffer)
{
  m_getCurLang.reset(new platform::GetTextById(jsonBuffer));
  ASSERT(m_getCurLang && m_getCurLang->IsValid(), ());
}

string GetTtsText::operator()(Notification const & notification) const
{
  if (notification.m_distanceUnits == 0 && !notification.m_useThenInsteadOfDistance)
    return GetTextById(GetDirectionTextId(notification));
  if (notification.m_useThenInsteadOfDistance && notification.m_turnDir == TurnDirection::NoTurn)
    return string();

  string const dirStr = GetTextById(GetDirectionTextId(notification));
  if (dirStr.empty())
    return string();

  string const distStr = GetTextById(GetDistanceTextId(notification));
  return distStr + " " + dirStr;
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
  if (notification.m_useThenInsteadOfDistance)
    return "then";

  switch (notification.m_lengthUnits)
  {
    case ::Settings::Metric:
      return DistToTextId(GetAllSoundedDistMeters().cbegin(), GetAllSoundedDistMeters().cend(),
                          notification.m_distanceUnits);
    case ::Settings::Foot:
      return DistToTextId(GetAllSoundedDistFeet().cbegin(), GetAllSoundedDistFeet().cend(),
                          notification.m_distanceUnits);
  }
  ASSERT(false, ());
  return string();
}

string GetRoundaboutTextId(Notification const & notification)
{
  if (notification.m_turnDir != TurnDirection::LeaveRoundAbout)
  {
    ASSERT(false, ());
    return string();
  }
  if (!notification.m_useThenInsteadOfDistance)
    return "leave_the_roundabout"; // Notification just before leaving a roundabout.

  static const uint8_t kMaxSoundedExit = 11;
  if (notification.m_exitNum == 0 || notification.m_exitNum > kMaxSoundedExit)
    return "leave_the_roundabout";

  if (notification.m_exitNum < 4)
  {
    switch (notification.m_exitNum)
    {
      case 1:
        return "take_the_1st_exit";
      case 2:
        return "take_the_2nd_exit";
      case 3:
        return "take_the_3rd_exit";
    }
    ASSERT(false, ());
    return string();
  }
  return "take_the_" + strings::to_string(static_cast<int>(notification.m_exitNum)) + "th_exit";
}

string GetDirectionTextId(Notification const & notification)
{
  switch (notification.m_turnDir)
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
      return GetRoundaboutTextId(notification);
    case TurnDirection::ReachedYourDestination:
      return "you_have_reached_the_destination";
    case TurnDirection::StayOnRoundAbout:
    case TurnDirection::StartAtEndOfStreet:
    case TurnDirection::TakeTheExit:
    case TurnDirection::NoTurn:
    case TurnDirection::Count:
      ASSERT(false, ());
      return string();
  }
  ASSERT(false, ());
  return string();
}
}  // namespace sound
}  // namespace turns
}  // namespace routing
