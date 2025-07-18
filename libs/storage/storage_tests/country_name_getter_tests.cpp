#include "testing/testing.hpp"

#include "storage/country_name_getter.hpp"

#include <string>

using namespace std;

UNIT_TEST(CountryNameGetterTest)
{
  string const shortJson =
      "\
      {\
      \"Russia_Moscow\":\"Москва\",\
      \"Russia_Rostov-on-Don\":\"Ростов-на-Дону\"\
      }";

  storage::CountryNameGetter getter;

  TEST_EQUAL(string(), getter.GetLocale(), ());
  TEST_EQUAL(string("Russia_Moscow"), getter("Russia_Moscow"), ());
  TEST_EQUAL(string("Russia_Rostov-on-Don"), getter("Russia_Rostov-on-Don"), ());
  TEST_EQUAL(string("Russia_Murmansk"), getter("Russia_Murmansk"), ());

  getter.SetLocaleForTesting(shortJson, "ru");

  TEST_EQUAL(string("ru"), getter.GetLocale(), ());
  TEST_EQUAL(string("Москва"), getter("Russia_Moscow"), ());
  TEST_EQUAL(string("Ростов-на-Дону"), getter("Russia_Rostov-on-Don"), ());
  TEST_EQUAL(string("Russia_Murmansk"), getter("Russia_Murmansk"), ());
}
