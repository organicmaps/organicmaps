#include "testing/testing.hpp"

#include "platform/get_text_by_id.hpp"

#include <string>

UNIT_TEST(GetTextByIdEnglishTest)
{
  std::string const shortJson =
      "\
      {\
      \"make_a_slight_right_turn\":\"Make a slight right turn.\",\
      \"in_900_meters\":\"In nine hundred meters.\",\
      \"then\":\"Then.\",\
      \"in_1_mile\":\"In one mile.\"\
      }";

  auto getEnglish = platform::ForTestingGetTextByIdFactory(shortJson, "en");
  TEST_EQUAL((*getEnglish)("make_a_slight_right_turn"), "Make a slight right turn.", ());
  TEST_EQUAL((*getEnglish)("in_900_meters"), "In nine hundred meters.", ());
  TEST_EQUAL((*getEnglish)("then"), "Then.", ());
  TEST_EQUAL((*getEnglish)("in_1_mile"), "In one mile.", ());

  TEST_EQUAL((*getEnglish)("some_nonexistent_key"), "", ());
  TEST_EQUAL((*getEnglish)("some_nonexistent_key"), "", ());
  TEST_EQUAL((*getEnglish)(""), "", ());
  TEST_EQUAL((*getEnglish)(" "), "", ());
}

UNIT_TEST(GetTextByIdRussianTest)
{
  std::string const shortJson =
      "\
      {\
      \"in_800_meters\":\"Через восемьсот метров.\",\
      \"make_a_slight_right_turn\":\"Держитесь правее.\",\
      \"take_the_6_exit\":\"Сверните на шестой съезд.\",\
      \"in_1_mile\":\"Через одну милю.\"\
      }";

  auto getRussian = platform::ForTestingGetTextByIdFactory(shortJson, "ru");
  TEST_EQUAL((*getRussian)("in_800_meters"), "Через восемьсот метров.", ());
  TEST_EQUAL((*getRussian)("make_a_slight_right_turn"), "Держитесь правее.", ());
  TEST_EQUAL((*getRussian)("take_the_6_exit"), "Сверните на шестой съезд.", ());
  TEST_EQUAL((*getRussian)("in_1_mile"), "Через одну милю.", ());

  TEST_EQUAL((*getRussian)("some_nonexistent_key"), "", ());
  TEST_EQUAL((*getRussian)("some_nonexistent_key"), "", ());
  TEST_EQUAL((*getRussian)(""), "", ());
  TEST_EQUAL((*getRussian)(" "), "", ());
}

UNIT_TEST(GetTextByIdKoreanTest)
{
  std::string const shortJson =
      "\
      {\
      \"in_700_meters\":\"700 미터 앞\",\
      \"make_a_right_turn\":\"우회전입니다.\",\
      \"take_the_5_exit\":\"다섯 번째 출구입니다.\",\
      \"in_5000_feet\":\"5000피트 앞\"\
      }";

  auto getKorean = platform::ForTestingGetTextByIdFactory(shortJson, "ko");
  TEST_EQUAL((*getKorean)("in_700_meters"), "700 미터 앞", ());
  TEST_EQUAL((*getKorean)("make_a_right_turn"), "우회전입니다.", ());
  TEST_EQUAL((*getKorean)("take_the_5_exit"), "다섯 번째 출구입니다.", ());
  TEST_EQUAL((*getKorean)("in_5000_feet"), "5000피트 앞", ());

  TEST_EQUAL((*getKorean)("some_nonexistent_key"), "", ());
  TEST_EQUAL((*getKorean)("some_nonexistent_key"), "", ());
  TEST_EQUAL((*getKorean)(""), "", ());
  TEST_EQUAL((*getKorean)(" "), "", ());
}

UNIT_TEST(GetTextByIdArabicTest)
{
  std::string const shortJson =
      "\
      {\
      \"in_1_kilometer\":\"بعد كيلو متر واحدٍ\",\
      \"leave_the_roundabout\":\"اخرج من الطريق الدوار\",\
      \"take_the_3_exit\":\"اسلك المخرج الثالث\",\
      \"in_4000_feet\":\"بعد 4000 قدم\"\
      }";

  auto getArabic = platform::ForTestingGetTextByIdFactory(shortJson, "ar");
  TEST_EQUAL((*getArabic)("in_1_kilometer"), "بعد كيلو متر واحدٍ", ());
  TEST_EQUAL((*getArabic)("leave_the_roundabout"), "اخرج من الطريق الدوار", ());
  TEST_EQUAL((*getArabic)("take_the_3_exit"), "اسلك المخرج الثالث", ());
  TEST_EQUAL((*getArabic)("in_4000_feet"), "بعد 4000 قدم", ());

  TEST_EQUAL((*getArabic)("some_nonexistent_key"), "", ());
  TEST_EQUAL((*getArabic)("some_nonexistent_key"), "", ());
  TEST_EQUAL((*getArabic)(""), "", ());
  TEST_EQUAL((*getArabic)(" "), "", ());
}

UNIT_TEST(GetTextByIdFrenchTest)
{
  std::string const shortJson =
      "\
      {\
      \"in_1_5_kilometers\":\"Dans un virgule cinq kilomètre.\",\
      \"enter_the_roundabout\":\"Prenez le rond-point.\",\
      \"take_the_2_exit\":\"Prenez la deuxième sortie.\",\
      \"in_3500_feet\":\"Dans trois mille cinq cents pieds.\"\
      }";

  auto getFrench = platform::ForTestingGetTextByIdFactory(shortJson, "fr");
  TEST_EQUAL((*getFrench)("in_1_5_kilometers"), "Dans un virgule cinq kilomètre.", ());
  TEST_EQUAL((*getFrench)("enter_the_roundabout"), "Prenez le rond-point.", ());
  TEST_EQUAL((*getFrench)("take_the_2_exit"), "Prenez la deuxième sortie.", ());
  TEST_EQUAL((*getFrench)("in_3500_feet"), "Dans trois mille cinq cents pieds.", ());

  TEST_EQUAL((*getFrench)("some_nonexistent_key"), "", ());
  TEST_EQUAL((*getFrench)("some_nonexistent_key"), "", ());
  TEST_EQUAL((*getFrench)(""), "", ());
  TEST_EQUAL((*getFrench)(" "), "", ());
}
