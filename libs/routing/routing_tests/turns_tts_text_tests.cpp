#include "testing/testing.hpp"

#include "routing/route.hpp"
#include "routing/turns_sound_settings.hpp"
#include "routing/turns_tts_text.hpp"
#include "routing/turns_tts_text_i18n.hpp"

#include "base/string_utils.hpp"

#include <cstring>
#include <string>

namespace turns_tts_text_tests
{
using namespace routing::turns;
using namespace routing::turns::sound;
using namespace std;
using namespace strings;

using measurement_utils::Units;

bool PairDistEquals(PairDist const & lhs, PairDist const & rhs)
{
  return lhs.first == rhs.first && strcmp(lhs.second, rhs.second) == 0;
}

UNIT_TEST(GetDistanceTextIdMetersTest)
{
  TEST_EQUAL(GetDistanceTextId({500, 0, false, CarDirection::TurnRight, Units::Metric}), "in_500_meters", ());

  //  Notification const notification2(500, 0, true, CarDirection::TurnRight, Units::Metric);
  //  TEST_EQUAL(GetDistanceTextId(notification2), "then", ());

  TEST_EQUAL(GetDistanceTextId({200, 0, false, CarDirection::TurnRight, Units::Metric}), "in_200_meters", ());
  TEST_EQUAL(GetDistanceTextId({2000, 0, false, CarDirection::TurnRight, Units::Metric}), "in_2_kilometers", ());

  /// @see DistToTextId.
  TEST_EQUAL(GetDistanceTextId({130, 0, false, CarDirection::TurnRight, Units::Metric}), "in_100_meters", ());
  TEST_EQUAL(GetDistanceTextId({135, 0, false, CarDirection::TurnRight, Units::Metric}), "in_200_meters", ());
}

UNIT_TEST(GetDistanceTextIdFeetTest)
{
  TEST_EQUAL(GetDistanceTextId({500, 0, false, CarDirection::TurnRight, Units::Imperial}), "in_500_feet", ());

  //  Notification const notification2(500, 0, true, CarDirection::TurnRight, Units::Imperial);
  //  TEST_EQUAL(GetDistanceTextId(notification2), "then", ());

  TEST_EQUAL(GetDistanceTextId({800, 0, false, CarDirection::TurnRight, Units::Imperial}), "in_800_feet", ());
  TEST_EQUAL(GetDistanceTextId({5000, 0, false, CarDirection::TurnRight, Units::Imperial}), "in_5000_feet", ());
}

UNIT_TEST(GetRoundaboutTextIdTest)
{
  // Notification(uint32_t distanceUnits, uint8_t exitNum, bool useThenInsteadOfDistance,
  //    CarDirection turnDir, ::Settings::Units lengthUnits)
  Notification const notification1(500, 0, false, CarDirection::LeaveRoundAbout, Units::Imperial);
  TEST_EQUAL(GetRoundaboutTextId(notification1), "leave_the_roundabout", ());
  Notification const notification2(0, 3, true, CarDirection::LeaveRoundAbout, Units::Imperial);
  TEST_EQUAL(GetRoundaboutTextId(notification2), "take_the_3_exit", ());
  Notification const notification3(0, 7, true, CarDirection::LeaveRoundAbout, Units::Metric);
  TEST_EQUAL(GetRoundaboutTextId(notification3), "take_the_7_exit", ());
  Notification const notification4(0, 15, true, CarDirection::LeaveRoundAbout, Units::Metric);
  TEST_EQUAL(GetRoundaboutTextId(notification4), "leave_the_roundabout", ());
}

UNIT_TEST(GetYouArriveTextIdTest)
{
  // Notification(uint32_t distanceUnits, uint8_t exitNum, bool useThenInsteadOfDistance,
  //    CarDirection turnDir, ::Settings::Units lengthUnits)
  Notification const notification1(500, 0, false, CarDirection::ReachedYourDestination, Units::Imperial);
  TEST_EQUAL(GetYouArriveTextId(notification1), "destination", ());
  Notification const notification2(0, 0, false, CarDirection::ReachedYourDestination, Units::Metric);
  TEST_EQUAL(GetYouArriveTextId(notification2), "you_have_reached_the_destination", ());
  Notification const notification3(0, 0, true, CarDirection::ReachedYourDestination, Units::Metric);
  TEST_EQUAL(GetYouArriveTextId(notification3), "destination", ());
}

UNIT_TEST(GetDirectionTextIdTest)
{
  // Notification(uint32_t distanceUnits, uint8_t exitNum, bool useThenInsteadOfDistance,
  //    CarDirection turnDir, ::Settings::Units lengthUnits)
  Notification const notification1(500, 0, false, CarDirection::TurnRight, Units::Imperial);
  TEST_EQUAL(GetDirectionTextId(notification1), "make_a_right_turn", ());
  Notification const notification2(1000, 0, false, CarDirection::GoStraight, Units::Metric);
  TEST_EQUAL(GetDirectionTextId(notification2), "go_straight", ());
  Notification const notification3(700, 0, false, CarDirection::UTurnLeft, Units::Metric);
  TEST_EQUAL(GetDirectionTextId(notification3), "make_a_u_turn", ());
  Notification const notification4(200, 0, false, CarDirection::ReachedYourDestination, Units::Metric);
  TEST_EQUAL(GetDirectionTextId(notification4), "destination", ());
  Notification const notification5(0, 0, false, CarDirection::ReachedYourDestination, Units::Metric);
  TEST_EQUAL(GetDirectionTextId(notification5), "you_have_reached_the_destination", ());
}

UNIT_TEST(GetTtsTextTest)
{
  string const engShortJson =
      R"({
      "in_300_meters":"In 300 meters.",
      "in_500_meters":"In 500 meters.",
      "then":"Then.",
      "make_a_right_turn":"Make a right turn.",
      "make_a_left_turn":"Make a left turn.",
      "you_have_reached_the_destination":"You have reached the destination."
      })";

  string const rusShortJson =
      R"({
      "in_300_meters":"Через 300 метров.",
      "in_500_meters":"Через 500 метров.",
      "then":"Затем.",
      "make_a_right_turn":"Поворот направо.",
      "make_a_left_turn":"Поворот налево.",
      "you_have_reached_the_destination":"Вы достигли конца маршрута."
      })";

  GetTtsText getTtsText;
  // Notification(uint32_t distanceUnits, uint8_t exitNum, bool useThenInsteadOfDistance,
  //    CarDirection turnDir, Settings::Units lengthUnits)
  Notification const notification1(500, 0, false, CarDirection::TurnRight, Units::Metric);
  Notification const notification2(300, 0, false, CarDirection::TurnLeft, Units::Metric);
  Notification const notification3(0, 0, false, CarDirection::ReachedYourDestination, Units::Metric);
  Notification const notification4(0, 0, true, CarDirection::TurnLeft, Units::Metric);

  getTtsText.ForTestingSetLocaleWithJson(engShortJson, "en");
  TEST_EQUAL(getTtsText.GetTurnNotification(notification1), "In 500 meters. Make a right turn.", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification2), "In 300 meters. Make a left turn.", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification3), "You have reached the destination.", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification4), "Then. Make a left turn.", ());

  getTtsText.ForTestingSetLocaleWithJson(rusShortJson, "ru");
  TEST_EQUAL(getTtsText.GetTurnNotification(notification1), "Через 500 метров. Поворот направо.", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification2), "Через 300 метров. Поворот налево.", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification3), "Вы достигли конца маршрута.", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification4), "Затем. Поворот налево.", ());
}

UNIT_TEST(EndsInAcronymOrNumTest)
{
  // actual JSON doesn't matter for this test
  string const huShortJson =
      R"({
      "in_300_meters":"Háromszáz méter után"
      })";

  GetTtsText getTtsText;
  getTtsText.ForTestingSetLocaleWithJson(huShortJson, "hu");
  TEST_EQUAL(EndsInAcronymOrNum(strings::MakeUniString("Main Street")), false, ());
  TEST_EQUAL(EndsInAcronymOrNum(strings::MakeUniString("Main STREET")), true, ());
  TEST_EQUAL(EndsInAcronymOrNum(strings::MakeUniString("Highway A")), true, ());
  TEST_EQUAL(EndsInAcronymOrNum(strings::MakeUniString("Highway AA")), true, ());
  TEST_EQUAL(EndsInAcronymOrNum(strings::MakeUniString("Highway Aa")), false, ());
  TEST_EQUAL(EndsInAcronymOrNum(strings::MakeUniString("Highway aA")), false, ());
  TEST_EQUAL(EndsInAcronymOrNum(strings::MakeUniString("Highway 50")), true, ());
  TEST_EQUAL(EndsInAcronymOrNum(strings::MakeUniString("Highway 50A")), true, ());
  TEST_EQUAL(EndsInAcronymOrNum(strings::MakeUniString("Highway A50")), true, ());
  TEST_EQUAL(EndsInAcronymOrNum(strings::MakeUniString("Highway a50")), false, ());
  TEST_EQUAL(EndsInAcronymOrNum(strings::MakeUniString("Highway 50a")), false, ());
  TEST_EQUAL(EndsInAcronymOrNum(strings::MakeUniString("50 Highway")), false, ());
  TEST_EQUAL(EndsInAcronymOrNum(strings::MakeUniString("AA Highway")), false, ());
}

UNIT_TEST(GetTtsStreetTextTest)
{
  // we can use "NULL" to indicate that the regular "_turn" string should be used and not "_turn_street"
  string const engShortJson =
      R"({
      "in_300_meters":"In 300 meters.",
      "in_500_meters":"In 500 meters.",
      "then":"Then.",
      "onto":"onto",
      "make_a_right_turn":"Make a right turn.",
      "make_a_left_turn":"Make a left turn.",
      "make_a_right_turn_street":"NULL",
      "make_a_left_turn_street":"NULL",
      "take_exit_number":"Take exit",
      "dist_direction_onto_street":"%1$s %2$s %3$s %4$s",
      "you_have_reached_the_destination":"You have reached the destination."
      })";

  string const jaShortJson =
      R"({
      "in_300_meters":"三百メートル先",
      "in_500_meters":"五百メートル先",
      "then":"その先",
      "onto":"に入ります",
      "make_a_right_turn":"右折です。",
      "make_a_left_turn":"左折です。",
      "make_a_right_turn_street":"右折し",
      "make_a_left_turn_street":"左折し",
      "dist_direction_onto_street":"%1$s%2$s %4$s %3$s",
      "you_have_reached_the_destination":"到着。"
      })";

  string const faShortJson =
      R"({
      "in_300_meters":"ﺩﺭ ﺲﯿﺻﺩ ﻢﺗﺮﯾ",
      "in_500_meters":"ﺩﺭ ﭖﺎﻨﺻﺩ ﻢﺗﺮﯾ",
      "then":"ﺲﭙﺳ",
      "onto":"ﺐﻫ",
      "make_a_right_turn":"ﺐﻫ ﺭﺎﺴﺗ ﺐﭙﯿﭽﯾﺩ.",
      "make_a_left_turn":"ﺐﻫ ﭻﭘ ﺐﭙﯿﭽﯾﺩ.",
      "dist_direction_onto_street":"%1$s %2$s %3$s %4$s",
      "you_have_reached_the_destination":"ﺶﻣﺍ ﺮﺴﯾﺪﻫ ﺎﯾﺩ."
      })";

  string const arShortJson =
      R"({
      "in_300_meters":"ﺐﻋﺩ ﺙﻼﺜﻤﺋﺓ ﻢﺗﺭ",
      "in_500_meters":"ﺐﻋﺩ ﺦﻤﺴﻤﺋﺓ ﻢﺗﺭ",
      "then":"ﺚﻣ",
      "onto":"ﺈﻟﻯ",
      "make_a_right_turn":"ﺎﻨﻌﻄﻓ ﻲﻤﻴﻧﺍ.",
      "make_a_left_turn":"ﺎﻨﻌﻄﻓ ﻲﺳﺍﺭﺍ.",
      "dist_direction_onto_street":"%1$s %2$s %3$s %4$s",
      "you_have_reached_the_destination":"ﻞﻗﺩ ﻮﺼﻠﺗ."
      })";

  string const huShortJson =
      R"({
      "in_300_meters":"Háromszáz méter után",
      "in_500_meters":"Ötszáz méter után",
      "go_straight":"Hajtson előre.",
      "then":"Majd",
      "onto":"a",
      "make_a_right_turn":"Forduljon jobbra.",
      "make_a_left_turn":"Forduljon balra.",
      "dist_direction_onto_street":"%1$s %2$s %3$s %4$s-re"
      })";

  string const nlShortJson =
      R"({
      "in_300_meters":"Over driehonderd meter",
      "in_500_meters":"Over vijfhonderd meter",
      "go_straight":"Rij rechtdoor.",
      "then":"Daarna",
      "onto":"naar",
      "make_a_right_turn":"Sla rechtsaf.",
      "make_a_right_turn_street":"naar rechts afslaan",
      "make_a_left_turn":"Sla linksaf.",
      "make_a_left_turn_street":"naar links afslaan",
      "dist_direction_onto_street":"%5$s %1$s %2$s %3$s %4$s",
      "take_exit_number":"Verlaat naar",
      "take_exit_number_street_verb":"Neem"
      })";

  GetTtsText getTtsText;
  // Notification(uint32_t distanceUnits, uint8_t exitNum, bool useThenInsteadOfDistance,
  //    CarDirection turnDir, Settings::Units lengthUnits, routing::RouteSegment::RoadNameInfo nextStreetInfo)

  Notification const notification1(500, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                   routing::RouteSegment::RoadNameInfo("Main Street"));
  Notification const notification2(300, 0, false, CarDirection::TurnLeft, measurement_utils::Units::Metric,
                                   routing::RouteSegment::RoadNameInfo("Main Street"));
  Notification const notification3(300, 0, false, CarDirection::TurnLeft, measurement_utils::Units::Metric);
  Notification const notification4(0, 0, true, CarDirection::TurnLeft, measurement_utils::Units::Metric);
  Notification const notification5(300, 0, false, CarDirection::TurnLeft, measurement_utils::Units::Metric,
                                   routing::RouteSegment::RoadNameInfo("Capital Parkway"));
  Notification const notification6(1000, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                   routing::RouteSegment::RoadNameInfo("Woodhaven Boulevard", "NY 25", "195"));
  Notification const notification7(1000, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                   routing::RouteSegment::RoadNameInfo("Woodhaven Boulevard", "NY 25", "1950"));

  getTtsText.ForTestingSetLocaleWithJson(engShortJson, "en");
  TEST_EQUAL(getTtsText.GetTurnNotification(notification1), "In 500 meters Make a right turn onto Main Street", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification2), "In 300 meters Make a left turn onto Main Street", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification3), "In 300 meters. Make a left turn.", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification4), "Then. Make a left turn.", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification6), "Take exit 195; NY 25; Woodhaven Boulevard", ());

  getTtsText.ForTestingSetLocaleWithJson(jaShortJson, "ja");
  TEST_EQUAL(getTtsText.GetTurnNotification(notification1), "五百メートル先右折し Main Street に入ります", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification2), "三百メートル先左折し Main Street に入ります", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification3), "三百メートル先左折です。", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification4), "その先左折です。", ());

  getTtsText.ForTestingSetLocaleWithJson(faShortJson, "fa");
  TEST_EQUAL(getTtsText.GetTurnNotification(notification1), "ﺩﺭ ﭖﺎﻨﺻﺩ ﻢﺗﺮﯾ ﺐﻫ ﺭﺎﺴﺗ ﺐﭙﯿﭽﯾﺩ ﺐﻫ Main Street", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification2), "ﺩﺭ ﺲﯿﺻﺩ ﻢﺗﺮﯾ ﺐﻫ ﭻﭘ ﺐﭙﯿﭽﯾﺩ ﺐﻫ Main Street", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification3), "ﺩﺭ ﺲﯿﺻﺩ ﻢﺗﺮﯾ ﺐﻫ ﭻﭘ ﺐﭙﯿﭽﯾﺩ.", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification4), "ﺲﭙﺳ ﺐﻫ ﭻﭘ ﺐﭙﯿﭽﯾﺩ.", ());

  getTtsText.ForTestingSetLocaleWithJson(arShortJson, "ar");
  TEST_EQUAL(getTtsText.GetTurnNotification(notification1), "ﺐﻋﺩ ﺦﻤﺴﻤﺋﺓ ﻢﺗﺭ ﺎﻨﻌﻄﻓ ﻲﻤﻴﻧﺍ ﺈﻟﻯ Main Street", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification2), "ﺐﻋﺩ ﺙﻼﺜﻤﺋﺓ ﻢﺗﺭ ﺎﻨﻌﻄﻓ ﻲﺳﺍﺭﺍ ﺈﻟﻯ Main Street", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification3), "ﺐﻋﺩ ﺙﻼﺜﻤﺋﺓ ﻢﺗﺭ ﺎﻨﻌﻄﻓ ﻲﺳﺍﺭﺍ.", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification4), "ﺚﻣ ﺎﻨﻌﻄﻓ ﻲﺳﺍﺭﺍ.", ());

  getTtsText.ForTestingSetLocaleWithJson(huShortJson, "hu");
  TEST_EQUAL(getTtsText.GetTurnNotification(notification1), "Ötszáz méter után Forduljon jobbra a Main Streetre", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification2), "Háromszáz méter után Forduljon balra a Main Streetre", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification3), "Háromszáz méter után Forduljon balra.", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification4), "Majd Forduljon balra.", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification5), "Háromszáz méter után Forduljon balra a Capital Parkwayra",
             ());  // -ra suffix for "back" vowel endings
  TEST_EQUAL(getTtsText.GetTurnNotification(notification6), "Forduljon jobbra a 195; NY 25; Woodhaven Boulevardra",
             ());  // a for prefixing "hundred ninety five"
  TEST_EQUAL(getTtsText.GetTurnNotification(notification7), "Forduljon jobbra az 1950; NY 25; Woodhaven Boulevardra",
             ());  // az for prefixing "thousand nine hundred fifty"
  Notification const notificationHuA(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                     routing::RouteSegment::RoadNameInfo("Woodhaven Boulevard", "NY 25", "19"));
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHuA),
             "Háromszáz méter után Forduljon jobbra a 19; NY 25; Woodhaven Boulevardra",
             ());  // a for prefixing "ten nine"
  Notification const notificationHuB(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                     routing::RouteSegment::RoadNameInfo("Woodhaven Boulevard", "NY 25", "1"));
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHuB),
             "Háromszáz méter után Forduljon jobbra az 1; NY 25; Woodhaven Boulevardra", ());  // az for prefixing "one"

  Notification const notificationHu1(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                     routing::RouteSegment::RoadNameInfo("puszta"));
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHu1), "Háromszáz méter után Forduljon jobbra a pusztára", ());
  Notification const notificationHu2(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                     routing::RouteSegment::RoadNameInfo("tanya"));
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHu2), "Háromszáz méter után Forduljon jobbra a tanyára", ());
  Notification const notificationHu3(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                     routing::RouteSegment::RoadNameInfo("utca"));
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHu3), "Háromszáz méter után Forduljon jobbra az utcára", ());
  Notification const notificationHu4(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                     routing::RouteSegment::RoadNameInfo("útja"));
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHu4), "Háromszáz méter után Forduljon jobbra az útjára", ());
  Notification const notificationHu5(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                     routing::RouteSegment::RoadNameInfo("allé"));
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHu5), "Háromszáz méter után Forduljon jobbra az allére",
             ());  // Some speakers say allére but it's personal judgment
  Notification const notificationHu6(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                     routing::RouteSegment::RoadNameInfo("útgyűrű"));
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHu6), "Háromszáz méter után Forduljon jobbra az útgyűrűre", ());
  Notification const notificationHu7(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                     routing::RouteSegment::RoadNameInfo("csirke"));
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHu7), "Háromszáz méter után Forduljon jobbra a csirkére", ());
  Notification const notificationHu8(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                     routing::RouteSegment::RoadNameInfo("1"));
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHu8), "Háromszáz méter után Forduljon jobbra az 1re", ());
  Notification const notificationHu8a(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                      routing::RouteSegment::RoadNameInfo("10"));
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHu8a), "Háromszáz méter után Forduljon jobbra a 10re", ());
  Notification const notificationHu9(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                     routing::RouteSegment::RoadNameInfo("20"));
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHu9), "Háromszáz méter után Forduljon jobbra a 20ra", ());
  Notification const notificationHu10(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                      routing::RouteSegment::RoadNameInfo("7"));
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHu10), "Háromszáz méter után Forduljon jobbra a 7re",
             ());  // 7 is re
  Notification const notificationHu11(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                      routing::RouteSegment::RoadNameInfo("71"));
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHu11), "Háromszáz méter után Forduljon jobbra a 71re",
             ());  // 70 is re and 1 is re
  Notification const notificationHu12(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                      routing::RouteSegment::RoadNameInfo("50"));
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHu12), "Háromszáz méter után Forduljon jobbra az 50re",
             ());  // 5* is az and 50 is re
  Notification const notificationHu13(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                      routing::RouteSegment::RoadNameInfo("Ybl utcá"));
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHu13), "Háromszáz méter után Forduljon jobbra az Ybl utcára",
             ());  // leading Y is a vowel
  Notification const notificationHu14(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                      routing::RouteSegment::RoadNameInfo("puszta "));
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHu14), "Háromszáz méter után Forduljon jobbra a pusztára",
             ());  // trailing space shouldn't matter
  Notification const notificationHu15(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                      routing::RouteSegment::RoadNameInfo(" puszta"));
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHu15), "Háromszáz méter után Forduljon jobbra a pusztára",
             ());  // leading space shouldn't matter
  Notification const notificationHu16(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                      routing::RouteSegment::RoadNameInfo(" "));
  // only spaces shouldn't matter, but it counts as an empty street name, thus not part of new TTS / Street Name /
  // Hungarian logic, thus having a terminal period.
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHu16), "Háromszáz méter után Forduljon jobbra.", ());
  Notification const notificationHu17(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                      routing::RouteSegment::RoadNameInfo("puszta; "));
  // semicolon doesn't affect the suffix, but it does introduce punctuation that stops the a -> á replacement. Hopefully
  // this is acceptable for now (odd case.)
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHu17), "Háromszáz méter után Forduljon jobbra a puszta;ra", ());
  Notification const notificationHu18(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                      routing::RouteSegment::RoadNameInfo("10Á"));
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHu18), "Háromszáz méter után Forduljon jobbra a 10Ára",
             ());  // 10* is a and Á is ra
  Notification const notificationHu19(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                      routing::RouteSegment::RoadNameInfo("5É"));
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHu19), "Háromszáz méter után Forduljon jobbra az 5Ére",
             ());  // 5* is az and É is re
  Notification const notificationHu20(300, 0, false, CarDirection::TurnRight, measurement_utils::Units::Metric,
                                      routing::RouteSegment::RoadNameInfo("É100"));
  TEST_EQUAL(getTtsText.GetTurnNotification(notificationHu20), "Háromszáz méter után Forduljon jobbra az É100ra",
             ());  // É* is az and 100 is ra

  getTtsText.ForTestingSetLocaleWithJson(nlShortJson, "nl");
  TEST_EQUAL(getTtsText.GetTurnNotification(notification1),
             "Over vijfhonderd meter naar rechts afslaan naar Main Street", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification2),
             "Over driehonderd meter naar links afslaan naar Main Street", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification3), "Over driehonderd meter Sla linksaf.", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification4), "Daarna Sla linksaf.", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification6), "Verlaat naar 195; NY 25; Woodhaven Boulevard", ());
}

UNIT_TEST(GetAllSoundedDistMetersTest)
{
  VecPairDist const & allSoundedDistMeters = GetAllSoundedDistMeters();

  TEST(is_sorted(allSoundedDistMeters.cbegin(), allSoundedDistMeters.cend(),
                 [](PairDist const & p1, PairDist const & p2) { return p1.first < p2.first; }),
       ());

  TEST_EQUAL(allSoundedDistMeters.size(), 17, ());
  PairDist const expected1 = {50, "in_50_meters"};
  TEST(PairDistEquals(allSoundedDistMeters[0], expected1), (allSoundedDistMeters[0], expected1));
  PairDist const expected2 = {700, "in_700_meters"};
  TEST(PairDistEquals(allSoundedDistMeters[8], expected2), (allSoundedDistMeters[8], expected2));
  PairDist const expected3 = {3000, "in_3_kilometers"};
  TEST(PairDistEquals(allSoundedDistMeters[16], expected3), (allSoundedDistMeters[16], expected3));
}

UNIT_TEST(GetAllSoundedDistFeet)
{
  VecPairDist const & allSoundedDistFeet = GetAllSoundedDistFeet();

  TEST(is_sorted(allSoundedDistFeet.cbegin(), allSoundedDistFeet.cend(),
                 [](PairDist const & p1, PairDist const & p2) { return p1.first < p2.first; }),
       ());

  TEST_EQUAL(allSoundedDistFeet.size(), 22, ());
  PairDist const expected1 = {50, "in_50_feet"};
  TEST(PairDistEquals(allSoundedDistFeet[0], expected1), (allSoundedDistFeet[0], expected1));
  PairDist const expected2 = {700, "in_700_feet"};
  TEST(PairDistEquals(allSoundedDistFeet[7], expected2), (allSoundedDistFeet[7], expected2));
  PairDist const expected3 = {10560, "in_2_miles"};
  TEST(PairDistEquals(allSoundedDistFeet[21], expected3), (allSoundedDistFeet[21], expected3));
}

UNIT_TEST(GetSoundedDistMeters)
{
  vector<uint32_t> const & soundedDistMeters = GetSoundedDistMeters();
  VecPairDist const & allSoundedDistMeters = GetAllSoundedDistMeters();

  TEST(is_sorted(soundedDistMeters.cbegin(), soundedDistMeters.cend()), ());
  // Checking that allSounded contains any element of inst.
  TEST(find_first_of(soundedDistMeters.cbegin(), soundedDistMeters.cend(), allSoundedDistMeters.cbegin(),
                     allSoundedDistMeters.cend(),
                     [](uint32_t p1, PairDist const & p2) { return p1 == p2.first; }) != soundedDistMeters.cend(),
       ());

  TEST_EQUAL(soundedDistMeters.size(), 11, ());
  TEST_EQUAL(soundedDistMeters[0], 200, ());
  TEST_EQUAL(soundedDistMeters[7], 900, ());
  TEST_EQUAL(soundedDistMeters[10], 2000, ());
}

UNIT_TEST(GetSoundedDistFeet)
{
  vector<uint32_t> soundedDistFeet = GetSoundedDistFeet();
  VecPairDist const & allSoundedDistFeet = GetAllSoundedDistFeet();

  TEST(is_sorted(soundedDistFeet.cbegin(), soundedDistFeet.cend()), ());
  // Checking that allSounded contains any element of inst.
  TEST(find_first_of(soundedDistFeet.cbegin(), soundedDistFeet.cend(), allSoundedDistFeet.cbegin(),
                     allSoundedDistFeet.cend(),
                     [](uint32_t p1, PairDist const & p2) { return p1 == p2.first; }) != soundedDistFeet.cend(),
       ());

  TEST_EQUAL(soundedDistFeet.size(), 11, ());
  TEST_EQUAL(soundedDistFeet[0], 500, ());
  TEST_EQUAL(soundedDistFeet[7], 2000, ());
  TEST_EQUAL(soundedDistFeet[10], 5000, ());
}
}  // namespace turns_tts_text_tests
