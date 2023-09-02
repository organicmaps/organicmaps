#include "testing/testing.hpp"

#include "routing/turns_sound_settings.hpp"
#include "routing/turns_tts_text.hpp"

#include <cstring>
#include <string>

namespace turns_tts_text_tests
{
using namespace routing::turns;
using namespace routing::turns::sound;
using namespace std;

bool PairDistEquals(PairDist const & lhs, PairDist const & rhs)
{
  return lhs.first == rhs.first && strcmp(lhs.second, rhs.second) == 0;
}

UNIT_TEST(GetDistanceTextIdMetersTest)
{
  // Notification(uint32_t distanceUnits, uint8_t exitNum, bool useThenInsteadOfDistance,
  //    CarDirection turnDir, ::Settings::Units lengthUnits)
  Notification const notification1(500, 0, false, CarDirection::TurnRight,
                                   measurement_utils::Units::Metric);
  TEST_EQUAL(GetDistanceTextId(notification1), "in_500_meters", ());
//  Notification const notification2(500, 0, true, CarDirection::TurnRight,
//                                   measurement_utils::Units::Metric);
//  TEST_EQUAL(GetDistanceTextId(notification2), "then", ());
  Notification const notification3(200, 0, false, CarDirection::TurnRight,
                                   measurement_utils::Units::Metric);
  TEST_EQUAL(GetDistanceTextId(notification3), "in_200_meters", ());
  Notification const notification4(2000, 0, false, CarDirection::TurnRight,
                                   measurement_utils::Units::Metric);
  TEST_EQUAL(GetDistanceTextId(notification4), "in_2_kilometers", ());
}

UNIT_TEST(GetDistanceTextIdFeetTest)
{
  // Notification(uint32_t distanceUnits, uint8_t exitNum, bool useThenInsteadOfDistance,
  //    CarDirection turnDir, ::Settings::Units lengthUnits)
  Notification const notification1(500, 0, false, CarDirection::TurnRight,
                                   measurement_utils::Units::Imperial);
  TEST_EQUAL(GetDistanceTextId(notification1), "in_500_feet", ());
//  Notification const notification2(500, 0, true, CarDirection::TurnRight,
//                                   measurement_utils::Units::Imperial);
//  TEST_EQUAL(GetDistanceTextId(notification2), "then", ());
  Notification const notification3(800, 0, false, CarDirection::TurnRight,
                                   measurement_utils::Units::Imperial);
  TEST_EQUAL(GetDistanceTextId(notification3), "in_800_feet", ());
  Notification const notification4(5000, 0, false, CarDirection::TurnRight,
                                   measurement_utils::Units::Imperial);
  TEST_EQUAL(GetDistanceTextId(notification4), "in_5000_feet", ());
}

UNIT_TEST(GetRoundaboutTextIdTest)
{
  // Notification(uint32_t distanceUnits, uint8_t exitNum, bool useThenInsteadOfDistance,
  //    CarDirection turnDir, ::Settings::Units lengthUnits)
  Notification const notification1(500, 0, false, CarDirection::LeaveRoundAbout,
                                   measurement_utils::Units::Imperial);
  TEST_EQUAL(GetRoundaboutTextId(notification1), "leave_the_roundabout", ());
  Notification const notification2(0, 3, true, CarDirection::LeaveRoundAbout,
                                   measurement_utils::Units::Imperial);
  TEST_EQUAL(GetRoundaboutTextId(notification2), "take_the_3_exit", ());
  Notification const notification3(0, 7, true, CarDirection::LeaveRoundAbout,
                                   measurement_utils::Units::Metric);
  TEST_EQUAL(GetRoundaboutTextId(notification3), "take_the_7_exit", ());
  Notification const notification4(0, 15, true, CarDirection::LeaveRoundAbout,
                                   measurement_utils::Units::Metric);
  TEST_EQUAL(GetRoundaboutTextId(notification4), "leave_the_roundabout", ());
}

UNIT_TEST(GetYouArriveTextIdTest)
{
  // Notification(uint32_t distanceUnits, uint8_t exitNum, bool useThenInsteadOfDistance,
  //    CarDirection turnDir, ::Settings::Units lengthUnits)
  Notification const notification1(500, 0, false, CarDirection::ReachedYourDestination,
                                   measurement_utils::Units::Imperial);
  TEST_EQUAL(GetYouArriveTextId(notification1), "destination", ());
  Notification const notification2(0, 0, false, CarDirection::ReachedYourDestination,
                                   measurement_utils::Units::Metric);
  TEST_EQUAL(GetYouArriveTextId(notification2), "you_have_reached_the_destination", ());
  Notification const notification3(0, 0, true, CarDirection::ReachedYourDestination,
                                   measurement_utils::Units::Metric);
  TEST_EQUAL(GetYouArriveTextId(notification3), "destination", ());
}

UNIT_TEST(GetDirectionTextIdTest)
{
  // Notification(uint32_t distanceUnits, uint8_t exitNum, bool useThenInsteadOfDistance,
  //    CarDirection turnDir, ::Settings::Units lengthUnits)
  Notification const notification1(500, 0, false, CarDirection::TurnRight,
                                   measurement_utils::Units::Imperial);
  TEST_EQUAL(GetDirectionTextId(notification1), "make_a_right_turn", ());
  Notification const notification2(1000, 0, false, CarDirection::GoStraight,
                                   measurement_utils::Units::Metric);
  TEST_EQUAL(GetDirectionTextId(notification2), "go_straight", ());
  Notification const notification3(700, 0, false, CarDirection::UTurnLeft,
                                   measurement_utils::Units::Metric);
  TEST_EQUAL(GetDirectionTextId(notification3), "make_a_u_turn", ());
  Notification const notification4(200, 0, false, CarDirection::ReachedYourDestination,
                                   measurement_utils::Units::Metric);
  TEST_EQUAL(GetDirectionTextId(notification4), "destination", ());
  Notification const notification5(0, 0, false, CarDirection::ReachedYourDestination,
                                   measurement_utils::Units::Metric);
  TEST_EQUAL(GetDirectionTextId(notification5), "you_have_reached_the_destination", ());
}

UNIT_TEST(GetTtsTextTest)
{
  string const engShortJson =
      "\
      {\
      \"in_300_meters\":\"In 300 meters.\",\
      \"in_500_meters\":\"In 500 meters.\",\
      \"then\":\"Then.\",\
      \"make_a_right_turn\":\"Make a right turn.\",\
      \"make_a_left_turn\":\"Make a left turn.\",\
      \"you_have_reached_the_destination\":\"You have reached the destination.\"\
      }";

  string const rusShortJson =
      "\
      {\
      \"in_300_meters\":\"Через 300 метров.\",\
      \"in_500_meters\":\"Через 500 метров.\",\
      \"then\":\"Затем.\",\
      \"make_a_right_turn\":\"Поворот направо.\",\
      \"make_a_left_turn\":\"Поворот налево.\",\
      \"you_have_reached_the_destination\":\"Вы достигли конца маршрута.\"\
      }";

  GetTtsText getTtsText;
  // Notification(uint32_t distanceUnits, uint8_t exitNum, bool useThenInsteadOfDistance,
  //    CarDirection turnDir, Settings::Units lengthUnits)
  Notification const notification1(500, 0, false, CarDirection::TurnRight,
                                   measurement_utils::Units::Metric);
  Notification const notification2(300, 0, false, CarDirection::TurnLeft,
                                   measurement_utils::Units::Metric);
  Notification const notification3(0, 0, false, CarDirection::ReachedYourDestination,
                                   measurement_utils::Units::Metric);
  Notification const notification4(0, 0, true, CarDirection::TurnLeft,
                                   measurement_utils::Units::Metric);

  getTtsText.ForTestingSetLocaleWithJson(engShortJson, "en");
  TEST_EQUAL(getTtsText.GetTurnNotification(notification1), "In 500 meters. Make a right turn.", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification2), "In 300 meters. Make a left turn.", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification3), "You have reached the destination.", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification4), "Then.  Make a left turn.", ());

  getTtsText.ForTestingSetLocaleWithJson(rusShortJson, "ru");
  TEST_EQUAL(getTtsText.GetTurnNotification(notification1), "Через 500 метров. Поворот направо.", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification2), "Через 300 метров. Поворот налево.", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification3), "Вы достигли конца маршрута.", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification4), "Затем.  Поворот налево.", ());
}

UNIT_TEST(GetTtsStreetTextTest)
{
  string const engShortJson =
      "\
      {\
      \"in_300_meters\":\"In 300 meters.\",\
      \"in_500_meters\":\"In 500 meters.\",\
      \"then\":\"Then.\",\
      \"onto\":\"onto\",\
      \"make_a_right_turn\":\"Make a right turn.\",\
      \"make_a_left_turn\":\"Make a left turn.\",\
      \"dist_direction_onto_street\":\"%1$s %2$s %3$s %4$s\",\
      \"you_have_reached_the_destination\":\"You have reached the destination.\"\
      }";

  string const jaShortJson =
      "\
      {\
      \"in_300_meters\":\"三百メートル先\",\
      \"in_500_meters\":\"五百メートル先\",\
      \"then\":\"その先\",\
      \"onto\":\"に入ります\",\
      \"make_a_right_turn\":\"右折です。\",\
      \"make_a_left_turn\":\"左折です。\",\
      \"make_a_right_turn_street\":\"右折し\",\
      \"make_a_left_turn_street\":\"左折し\",\
      \"dist_direction_onto_street\":\"%1$s%2$s %4$s %3$s\",\
      \"you_have_reached_the_destination\":\"到着。\"\
      }";

  string const faShortJson =
      "\
      {\
      \"in_300_meters\":\"ﺩﺭ ﺲﯿﺻﺩ ﻢﺗﺮﯾ\",\
      \"in_500_meters\":\"ﺩﺭ ﭖﺎﻨﺻﺩ ﻢﺗﺮﯾ\",\
      \"then\":\"ﺲﭙﺳ\",\
      \"onto\":\"ﺐﻫ\",\
      \"make_a_right_turn\":\"ﺐﻫ ﺭﺎﺴﺗ ﺐﭙﯿﭽﯾﺩ.\",\
      \"make_a_left_turn\":\"ﺐﻫ ﭻﭘ ﺐﭙﯿﭽﯾﺩ.\",\
      \"dist_direction_onto_street\":\"%1$s %2$s %3$s %4$s\",\
      \"you_have_reached_the_destination\":\"ﺶﻣﺍ ﺮﺴﯾﺪﻫ ﺎﯾﺩ.\"\
      }";

  string const arShortJson =
      "\
      {\
      \"in_300_meters\":\"ﺐﻋﺩ ﺙﻼﺜﻤﺋﺓ ﻢﺗﺭ\",\
      \"in_500_meters\":\"ﺐﻋﺩ ﺦﻤﺴﻤﺋﺓ ﻢﺗﺭ\",\
      \"then\":\"ﺚﻣ\",\
      \"onto\":\"ﺈﻟﻯ\",\
      \"make_a_right_turn\":\"ﺎﻨﻌﻄﻓ ﻲﻤﻴﻧﺍ.\",\
      \"make_a_left_turn\":\"ﺎﻨﻌﻄﻓ ﻲﺳﺍﺭﺍ.\",\
      \"dist_direction_onto_street\":\"%1$s %2$s %3$s %4$s\",\
      \"you_have_reached_the_destination\":\"ﻞﻗﺩ ﻮﺼﻠﺗ.\"\
      }";

  GetTtsText getTtsText;
  // Notification(uint32_t distanceUnits, uint8_t exitNum, bool useThenInsteadOfDistance,
  //    CarDirection turnDir, Settings::Units lengthUnits, std::string nextStreet)
  Notification const notification1(500, 0, false, CarDirection::TurnRight,
                                   measurement_utils::Units::Metric, "Main Street");
  Notification const notification2(300, 0, false, CarDirection::TurnLeft,
                                   measurement_utils::Units::Metric, "Main Street");
  Notification const notification3(300, 0, false, CarDirection::TurnLeft,
                                   measurement_utils::Units::Metric);
  Notification const notification4(0, 0, true, CarDirection::TurnLeft,
                                   measurement_utils::Units::Metric);

  getTtsText.ForTestingSetLocaleWithJson(engShortJson, "en");
  TEST_EQUAL(getTtsText.GetTurnNotification(notification1), "In 500 meters Make a right turn onto Main Street", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification2), "In 300 meters Make a left turn onto Main Street", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification3), "In 300 meters. Make a left turn.", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification4), "Then.  Make a left turn.", ());

  getTtsText.ForTestingSetLocaleWithJson(jaShortJson, "ja");
  TEST_EQUAL(getTtsText.GetTurnNotification(notification1), "五百メートル先右折し Main Street に入ります", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification2), "三百メートル先左折し Main Street に入ります", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification3), "三百メートル先 左折です。", ()); // note the extraneous space here due to + " " +
  TEST_EQUAL(getTtsText.GetTurnNotification(notification4), "その先  左折です。", ()); // note the extraneous spaces here due to + " " +

  getTtsText.ForTestingSetLocaleWithJson(faShortJson, "fa");
  TEST_EQUAL(getTtsText.GetTurnNotification(notification1), "ﺩﺭ ﭖﺎﻨﺻﺩ ﻢﺗﺮﯾ ﺐﻫ ﺭﺎﺴﺗ ﺐﭙﯿﭽﯾﺩ ﺐﻫ Main Street", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification2), "ﺩﺭ ﺲﯿﺻﺩ ﻢﺗﺮﯾ ﺐﻫ ﭻﭘ ﺐﭙﯿﭽﯾﺩ ﺐﻫ Main Street", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification3), "ﺩﺭ ﺲﯿﺻﺩ ﻢﺗﺮﯾ ﺐﻫ ﭻﭘ ﺐﭙﯿﭽﯾﺩ.", ()); // note the extraneous space here due to + " " +
  TEST_EQUAL(getTtsText.GetTurnNotification(notification4), "ﺲﭙﺳ  ﺐﻫ ﭻﭘ ﺐﭙﯿﭽﯾﺩ.", ()); // note the extraneous spaces here due to + " " +

  getTtsText.ForTestingSetLocaleWithJson(arShortJson, "ar");
  //TEST_EQUAL(getTtsText.GetTurnNotification(notification1), "بعد ستمئة قدم انعطف يمينا ﺈﻟﻯ Main Street", ());
  //TEST_EQUAL(getTtsText.GetTurnNotification(notification1), "بع ﺙﻼﺜﻤمئة قدم انعطف يمينا ﺈﻟﻯ Main Street", ());
  TEST_EQUAL(getTtsText.GetTurnNotification(notification3), "ﺐﻋﺩ ﺙﻼﺜﻤﺋﺓ ﻢﺗﺭ ﺎﻨﻌﻄﻓ ﻲﺳﺍﺭﺍ.", ()); // note the extraneous space here due to + " " +
  TEST_EQUAL(getTtsText.GetTurnNotification(notification4), "ﺚﻣ  ﺎﻨﻌﻄﻓ ﻲﺳﺍﺭﺍ.", ()); // note the extraneous spaces here due to + " " +

  // TODO: make tests for Dutch (nl) with verb prefixes, and for new "take exit 123" syntax
}

UNIT_TEST(GetAllSoundedDistMetersTest)
{
  VecPairDist const & allSoundedDistMeters = GetAllSoundedDistMeters();

  TEST(is_sorted(allSoundedDistMeters.cbegin(), allSoundedDistMeters.cend(),
                 [](PairDist const & p1, PairDist const & p2)
       {
         return p1.first < p2.first;
       }), ());

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
                 [](PairDist const & p1, PairDist const & p2)
       {
         return p1.first < p2.first;
       }), ());

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
  TEST(find_first_of(soundedDistMeters.cbegin(), soundedDistMeters.cend(),
                     allSoundedDistMeters.cbegin(), allSoundedDistMeters.cend(),
                     [](uint32_t p1, PairDist const & p2)
       {
         return p1 == p2.first;
       }) != soundedDistMeters.cend(), ());

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
                     allSoundedDistFeet.cend(), [](uint32_t p1, PairDist const & p2)
       {
         return p1 == p2.first;
       }) != soundedDistFeet.cend(), ());

  TEST_EQUAL(soundedDistFeet.size(), 11, ());
  TEST_EQUAL(soundedDistFeet[0], 500, ());
  TEST_EQUAL(soundedDistFeet[7], 2000, ());
  TEST_EQUAL(soundedDistFeet[10], 5000, ());
}
}  // namespace turns_tts_text_tests
