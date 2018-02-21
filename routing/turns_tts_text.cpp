#include "routing/turns_sound_settings.hpp"
#include "routing/turns_tts_text.hpp"

#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/iterator.hpp"
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
  m_getCurLang = platform::GetTextByIdFactory(platform::TextSource::TtsSound, locale);
}

void GetTtsText::ForTestingSetLocaleWithJson(string const & jsonBuffer, string const & locale)
{
  m_getCurLang = platform::ForTestingGetTextByIdFactory(jsonBuffer, locale);
}

string GetTtsText::operator()(Notification const & notification) const
{
  if (notification.m_distanceUnits == 0 && !notification.m_useThenInsteadOfDistance)
    return GetTextById(GetDirectionTextId(notification));
  if (notification.m_useThenInsteadOfDistance && notification.m_turnDir == CarDirection::None)
    return string();

  string const dirStr = GetTextById(GetDirectionTextId(notification));
  if (dirStr.empty())
    return string();

  string const distStr = GetTextById(GetDistanceTextId(notification));
  return distStr + " " + dirStr;
}

string GetTtsText::GetLocale() const
{
  if (m_getCurLang == nullptr)
  {
    ASSERT(false, ());
    return string();
  }
  return m_getCurLang->GetLocale();
}

string GetTtsText::GetTextById(string const & textId) const
{
  ASSERT(!textId.empty(), ());

  if (m_getCurLang == nullptr)
  {
    ASSERT(false, ());
    return string();
  }
  return (*m_getCurLang)(textId);
}

string GetDistanceTextId(Notification const & notification)
{
  if (notification.m_useThenInsteadOfDistance)
    return "then";

  switch (notification.m_lengthUnits)
  {
  case measurement_utils::Units::Metric:
    return DistToTextId(GetAllSoundedDistMeters().cbegin(), GetAllSoundedDistMeters().cend(),
                        notification.m_distanceUnits);
  case measurement_utils::Units::Imperial:
    return DistToTextId(GetAllSoundedDistFeet().cbegin(), GetAllSoundedDistFeet().cend(),
                        notification.m_distanceUnits);
  }
  ASSERT(false, ());
  return string();
}

string GetRoundaboutTextId(Notification const & notification)
{
  if (notification.m_turnDir != CarDirection::LeaveRoundAbout)
  {
    ASSERT(false, ());
    return string();
  }
  if (!notification.m_useThenInsteadOfDistance)
    return "leave_the_roundabout"; // Notification just before leaving a roundabout.

  static const uint8_t kMaxSoundedExit = 11;
  if (notification.m_exitNum == 0 || notification.m_exitNum > kMaxSoundedExit)
    return "leave_the_roundabout";

  return "take_the_" + strings::to_string(static_cast<int>(notification.m_exitNum)) + "_exit";
}

string GetYouArriveTextId(Notification const & notification)
{
  if (notification.m_turnDir != CarDirection::ReachedYourDestination)
  {
    ASSERT(false, ());
    return string();
  }

  if (notification.m_distanceUnits != 0 || notification.m_useThenInsteadOfDistance)
    return "destination";
  return "you_have_reached_the_destination";
}

string GetDirectionTextId(Notification const & notification)
{
  switch (notification.m_turnDir)
  {
    case CarDirection::GoStraight:
      return "go_straight";
    case CarDirection::TurnRight:
      return "make_a_right_turn";
    case CarDirection::TurnSharpRight:
      return "make_a_sharp_right_turn";
    case CarDirection::TurnSlightRight:
      return "make_a_slight_right_turn";
    case CarDirection::TurnLeft:
      return "make_a_left_turn";
    case CarDirection::TurnSharpLeft:
      return "make_a_sharp_left_turn";
    case CarDirection::TurnSlightLeft:
      return "make_a_slight_left_turn";
    case CarDirection::UTurnLeft:
    case CarDirection::UTurnRight:
      return "make_a_u_turn";
    case CarDirection::EnterRoundAbout:
      return "enter_the_roundabout";
    case CarDirection::LeaveRoundAbout:
      return GetRoundaboutTextId(notification);
    case CarDirection::ReachedYourDestination:
      return GetYouArriveTextId(notification);
    case CarDirection::StayOnRoundAbout:
    case CarDirection::StartAtEndOfStreet:
    case CarDirection::ExitHighwayToRight:
    case CarDirection::ExitHighwayToLeft:
    case CarDirection::None:
    case CarDirection::Count:
      ASSERT(false, ());
      return string();
  }
  ASSERT(false, ());
  return string();
}
}  // namespace sound
}  // namespace turns
}  // namespace routing
