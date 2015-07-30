#include "testing/testing.hpp"

#include "platform/get_text_by_id.hpp"

using namespace platform;

UNIT_TEST(GetTextByIdEnglishTest)
{
  platform::GetTextById getEnglish(TextSource::TtsSound, "en");
  TEST_EQUAL(getEnglish("make_a_slight_right_turn"), "Make a slight right turn.", ());
  TEST_EQUAL(getEnglish("in_900_meters"), "In 900 meters.", ());
  TEST_EQUAL(getEnglish("then"), "Then.", ());
  TEST_EQUAL(getEnglish("in_1_mile"), "In one mile.", ());
  TEST_EQUAL(getEnglish("some_nonexistent_key"), "some_nonexistent_key", ());
}

UNIT_TEST(GetTextByIdRussianTest)
{
  platform::GetTextById getRussian(TextSource::TtsSound, "ru");
  TEST_EQUAL(getRussian("in_800_meters"), "Через 800 метров.", ());
  TEST_EQUAL(getRussian("make_a_slight_right_turn"), "Плавный поворот направо.", ());
  TEST_EQUAL(getRussian("take_the_6th_exit"), "6-й поворот с кольца.", ());
  TEST_EQUAL(getRussian("in_1_mile"), "Через одну милю.", ());
  TEST_EQUAL(getRussian("some_nonexistent_key"), "some_nonexistent_key", ());
}

UNIT_TEST(GetTextByIdKoreanTest)
{
  platform::GetTextById getKorean(TextSource::TtsSound, "ko");
  TEST_EQUAL(getKorean("in_700_meters"), "700 미터 앞", ());
  TEST_EQUAL(getKorean("make_a_right_turn"), "우회전입니다.", ());
  TEST_EQUAL(getKorean("take_the_5th_exit"), "다섯 번째 출구입니다.", ());
  TEST_EQUAL(getKorean("in_5000_feet"), "5000피트 앞", ());
  TEST_EQUAL(getKorean("some_nonexistent_key"), "some_nonexistent_key", ());
}

UNIT_TEST(GetTextByIdArabicTest)
{
  platform::GetTextById getArabic(TextSource::TtsSound, "ar");
  TEST_EQUAL(getArabic("in_1_kilometer"), "بعد كيلو متر واحدٍ", ());
  TEST_EQUAL(getArabic("leave_the_roundabout"), "اخرج من الطريق الدوار", ());
  TEST_EQUAL(getArabic("take_the_3rd_exit"), "اسلك المخرج الثالث", ());
  TEST_EQUAL(getArabic("in_4000_feet"), "بعد 4000 قدم", ());
  TEST_EQUAL(getArabic("some_nonexistent_key"), "some_nonexistent_key", ());
}

UNIT_TEST(GetTextByIdFrenchTest)
{
  platform::GetTextById getFrench(TextSource::TtsSound, "fr");
  TEST_EQUAL(getFrench("in_1_5_kilometers"), "Dans un virgule cinq kilomètre.", ());
  TEST_EQUAL(getFrench("enter_the_roundabout"), "Prenez le rond-point.", ());
  TEST_EQUAL(getFrench("take_the_2nd_exit"), "Prenez la deuxième sortie.", ());
  TEST_EQUAL(getFrench("in_3500_feet"), "Dans 3500 feet.", ());
  TEST_EQUAL(getFrench("some_nonexistent_key"), "some_nonexistent_key", ());
}
