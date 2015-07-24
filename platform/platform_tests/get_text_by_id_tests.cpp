#include "testing/testing.hpp"

#include "platform/get_text_by_id.hpp"

UNIT_TEST(GetTextByIdEnglishTest)
{
  platform::GetTextById getEng("../data/sound-strings", "en");
  TEST_EQUAL(getEng("make_a_slight_right_turn"), "Make a slight right turn.", ());
  TEST_EQUAL(getEng("in_900_meters"), "In 900 meters.", ());
  TEST_EQUAL(getEng("then"), "Then.", ());
  TEST_EQUAL(getEng("in_one_mile"), "In one mile.", ());
}

UNIT_TEST(GetTextByIdRussianTest)
{
  platform::GetTextById getRussian("../data/sound-strings", "ru");
  TEST_EQUAL(getRussian("in_800_meters"), "Через 800 метров.", ());
  TEST_EQUAL(getRussian("make_a_slight_right_turn"), "Плавный поворот направо.", ());
  TEST_EQUAL(getRussian("take_the_6th_exit"), "6-й поворот с кольца.", ());
  TEST_EQUAL(getRussian("in_one_mile"), "Через одну милю.", ());
}

UNIT_TEST(GetTextByIdKoreanTest)
{
  platform::GetTextById getKorean("../data/sound-strings", "ko");
  TEST_EQUAL(getKorean("in_700_meters"), "700 미터 앞", ());
  TEST_EQUAL(getKorean("make_a_right_turn"), "우회전입니다.", ());
  TEST_EQUAL(getKorean("take_the_5th_exit"), "다섯 번째 출구입니다.", ());
  TEST_EQUAL(getKorean("in_5000_feet"), "5000피트 앞", ());
}

UNIT_TEST(GetTextByIdArabicTest)
{
  platform::GetTextById getArabic("../data/sound-strings", "ar");
  TEST_EQUAL(getArabic("in_one_kilometer"), "بعد كيلو متر واحدٍ", ());
  TEST_EQUAL(getArabic("leave_the_roundabout"), "اخرج من الطريق الدوار", ());
  TEST_EQUAL(getArabic("take_the_third_exit"), "اسلك المخرج الثالث", ());
  TEST_EQUAL(getArabic("in_4000_feet"), "بعد 4000 قدم", ());
}

UNIT_TEST(GetTextByIdFrenchTest)
{
  platform::GetTextById getFrench("../data/sound-strings", "fr");
  TEST_EQUAL(getFrench("in_one_and_a_half_kilometer"), "Dans un virgule cinq kilomètre.", ());
  TEST_EQUAL(getFrench("enter_the_roundabout"), "Prenez le rond-point.", ());
  TEST_EQUAL(getFrench("take_the_second_exit"), "Prenez la deuxième sortie.", ());
  TEST_EQUAL(getFrench("in_3500_feet"), "Dans 3500 feet.", ());
}
