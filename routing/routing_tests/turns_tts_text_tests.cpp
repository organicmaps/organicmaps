#include "testing/testing.hpp"

#include "routing/turns_sound_settings.hpp"
#include "routing/turns_tts_text.hpp"

namespace
{
using namespace routing::turns;
using namespace routing::turns::sound;

UNIT_TEST(GetDistanceTextIdMetersTest)
{
  //Notification(uint32_t distanceUnits, uint8_t exitNum, bool useThenInsteadOfDistance,
  //    TurnDirection turnDir, LengthUnits lengthUnits)
  Notification const notifiation1(500, 0, false, TurnDirection::TurnRight, LengthUnits::Meters);
  TEST_EQUAL(GetDistanceTextId(notifiation1), "in_500_meters", ());
  Notification const notifiation2(500, 0, true, TurnDirection::TurnRight, LengthUnits::Meters);
  TEST_EQUAL(GetDistanceTextId(notifiation2), "then", ());
  Notification const notifiation3(200, 0, false, TurnDirection::TurnRight, LengthUnits::Meters);
  TEST_EQUAL(GetDistanceTextId(notifiation3), "in_200_meters", ());
  Notification const notifiation4(2000, 0, false, TurnDirection::TurnRight, LengthUnits::Meters);
  TEST_EQUAL(GetDistanceTextId(notifiation4), "in_2_kilometers", ());
}

UNIT_TEST(GetDistanceTextIdFeetTest)
{
  Notification const notifiation1(500, 0, false, TurnDirection::TurnRight, LengthUnits::Feet);
  TEST_EQUAL(GetDistanceTextId(notifiation1), "in_500_feet", ());
  Notification const notifiation2(500, 0, true, TurnDirection::TurnRight, LengthUnits::Feet);
  TEST_EQUAL(GetDistanceTextId(notifiation2), "then", ());
  Notification const notifiation3(800, 0, false, TurnDirection::TurnRight, LengthUnits::Feet);
  TEST_EQUAL(GetDistanceTextId(notifiation3), "in_800_feet", ());
  Notification const notifiation4(5000, 0, false, TurnDirection::TurnRight, LengthUnits::Feet);
  TEST_EQUAL(GetDistanceTextId(notifiation4), "in_5000_feet", ());
}

UNIT_TEST(GetDirectionTextIdTest)
{
  Notification const notifiation1(500, 0, false, TurnDirection::TurnRight, LengthUnits::Feet);
  TEST_EQUAL(GetDirectionTextId(notifiation1), "make_a_right_turn", ());
  Notification const notifiation2(1000, 0, false, TurnDirection::GoStraight, LengthUnits::Meters);
  TEST_EQUAL(GetDirectionTextId(notifiation2), "go_straight", ());
  Notification const notifiation3(700, 0, false, TurnDirection::UTurn, LengthUnits::Meters);
  TEST_EQUAL(GetDirectionTextId(notifiation3), "make_a_u_turn", ());
}

UNIT_TEST(GetTtsTextTest)
{
  //Notification(uint32_t distanceUnits, uint8_t exitNum, bool useThenInsteadOfDistance,
  //    TurnDirection turnDir, LengthUnits lengthUnits)
  GetTtsText getTtsText;

  Notification const notifiation1(500, 0, false, TurnDirection::TurnRight, LengthUnits::Meters);
  Notification const notifiation2(300, 0, false, TurnDirection::TurnLeft, LengthUnits::Meters);
  Notification const notifiation3(0, 0, false, TurnDirection::ReachedYourDestination,
                                  LengthUnits::Meters);

  getTtsText.SetLocale("en");
  TEST_EQUAL(getTtsText(notifiation1), "In 500 meters. Make a right turn.", ());
  TEST_EQUAL(getTtsText(notifiation2), "In 300 meters. Make a left turn.", ());
  TEST_EQUAL(getTtsText(notifiation3), "You have reached the destination.", ());

  getTtsText.SetLocale("ru");
  TEST_EQUAL(getTtsText(notifiation1), "Через 500 метров. Поворот направо.", ());
  TEST_EQUAL(getTtsText(notifiation2), "Через 300 метров. Поворот налево.", ());
  TEST_EQUAL(getTtsText(notifiation3), "Вы достигли конца маршрута.", ());
}
} //  namespace
