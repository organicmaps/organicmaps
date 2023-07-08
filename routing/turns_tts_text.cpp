#include "routing/turns_sound_settings.hpp"
#include "routing/turns_tts_text.hpp"

#include "base/string_utils.hpp"

#include <algorithm>
#include <iterator>
#include <string>

namespace routing
{
namespace turns
{
namespace sound
{

namespace
{
using namespace routing::turns::sound;

template <class TIter> std::string DistToTextId(TIter begin, TIter end, uint32_t dist)
{
  using TValue = typename std::iterator_traits<TIter>::value_type;

  TIter distToSound = lower_bound(begin, end, dist, [](TValue const & p1, uint32_t p2)
                      {
                        return p1.first < p2;
                      });
  if (distToSound == end)
  {
    ASSERT(false, ("notification.m_distanceUnits is not correct."));
    return std::string{};
  }
  return distToSound->second;
}
}  //  namespace

void GetTtsText::SetLocale(std::string const & locale)
{
  m_getCurLang = platform::GetTextByIdFactory(platform::TextSource::TtsSound, locale);
}

void GetTtsText::ForTestingSetLocaleWithJson(std::string const & jsonBuffer, std::string const & locale)
{
  m_getCurLang = platform::ForTestingGetTextByIdFactory(jsonBuffer, locale);
}

std::string GetTtsText::GetTurnNotification(Notification const & notification) const
{
  if (notification.m_distanceUnits == 0 && !notification.m_useThenInsteadOfDistance)
    return GetTextById(GetDirectionTextId(notification));

  if (notification.IsPedestrianNotification())
  {
    if (notification.m_useThenInsteadOfDistance &&
        notification.m_turnDirPedestrian == PedestrianDirection::None)
      return {};
  }

  if (notification.m_useThenInsteadOfDistance && notification.m_turnDir == CarDirection::None)
    return {};

  std::string const dirStr = GetTextById(GetDirectionTextId(notification));
  if (dirStr.empty())
    return {};

  std::string distStr;
  if (notification.m_useThenInsteadOfDistance)
  {
    distStr = GetTextById("then");
    if (notification.m_distanceUnits > 0)
      distStr = distStr + " " + GetTextById(GetDistanceTextId(notification));
  }
  else
    distStr = GetTextById(GetDistanceTextId(notification));
  return distStr + " " + dirStr;
}

std::string GetTtsText::GetSpeedCameraNotification() const
{
  return GetTextById("unknown_camera");
}

std::string GetTtsText::GetLocale() const
{
  if (m_getCurLang == nullptr)
  {
    ASSERT(false, ());
    return {};
  }
  return m_getCurLang->GetLocale();
}

std::string GetTtsText::GetTextById(std::string const & textId) const
{
  ASSERT(!textId.empty(), ());

  if (m_getCurLang == nullptr)
  {
    ASSERT(false, ());
    return {};
  }
  return (*m_getCurLang)(textId);
}

std::string GetDistanceTextId(Notification const & notification)
{
//  if (notification.m_useThenInsteadOfDistance)
//    return "then";

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
  return {};
}

std::string GetRoundaboutTextId(Notification const & notification)
{
  if (notification.m_turnDir != CarDirection::LeaveRoundAbout)
  {
    ASSERT(false, ());
    return std::string{};
  }
  if (!notification.m_useThenInsteadOfDistance)
    return "leave_the_roundabout"; // Notification just before leaving a roundabout.

  static const uint8_t kMaxSoundedExit = 11;
  if (notification.m_exitNum == 0 || notification.m_exitNum > kMaxSoundedExit)
    return "leave_the_roundabout";

  return "take_the_" + strings::to_string(static_cast<int>(notification.m_exitNum)) + "_exit";
}

std::string GetYouArriveTextId(Notification const & notification)
{
  if (!notification.IsPedestrianNotification() &&
      notification.m_turnDir != CarDirection::ReachedYourDestination)
  {
    ASSERT(false, ());
    return std::string{};
  }

  if (notification.IsPedestrianNotification() &&
      notification.m_turnDirPedestrian != PedestrianDirection::ReachedYourDestination)
  {
    ASSERT(false, ());
    return std::string{};
  }

  if (notification.m_distanceUnits != 0 || notification.m_useThenInsteadOfDistance)
    return "destination";
  return "you_have_reached_the_destination";
}

std::string GetDirectionTextId(Notification const & notification)
{
  if (notification.IsPedestrianNotification())
  {
    switch (notification.m_turnDirPedestrian)
    {
    case PedestrianDirection::GoStraight: return "go_straight";
    case PedestrianDirection::TurnRight: return "make_a_right_turn";
    case PedestrianDirection::TurnLeft: return "make_a_left_turn";
    case PedestrianDirection::ReachedYourDestination: return GetYouArriveTextId(notification);
    case PedestrianDirection::None:
    case PedestrianDirection::Count: ASSERT(false, (notification)); return std::string{};
    }
  }

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
    case CarDirection::ExitHighwayToLeft:
    case CarDirection::ExitHighwayToRight:
      return "exit";
    case CarDirection::StayOnRoundAbout:
    case CarDirection::StartAtEndOfStreet:
    case CarDirection::None:
    case CarDirection::Count:
      ASSERT(false, ());
      return std::string{};
  }
  ASSERT(false, ());
  return std::string{};
}
}  // namespace sound
}  // namespace turns
}  // namespace routing
