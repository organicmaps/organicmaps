#include "testing/testing.hpp"

#include "storage/country_name_getter.hpp"

#include <string>

UNIT_TEST(CountryNameGetterTest)
{
  std::string const shortJson =
      "\
      {\
      \"Russia_Moscow\":\"Москва\",\
      \"Russia_Rostov-on-Don\":\"Ростов-на-Дону\"\
      }";

  storage::CountryNameGetter getter;

  TEST_EQUAL(std::string(), getter.GetLocale(), ());
  TEST_EQUAL(std::string("Russia_Moscow"), getter("Russia_Moscow"), ());
  TEST_EQUAL(std::string("Russia_Rostov-on-Don"), getter("Russia_Rostov-on-Don"), ());
  TEST_EQUAL(std::string("Russia_Murmansk"), getter("Russia_Murmansk"), ());

  getter.SetLocaleForTesting(shortJson, "ru");

  TEST_EQUAL(std::string("ru"), getter.GetLocale(), ());
  TEST_EQUAL(std::string("Москва"), getter("Russia_Moscow"), ());
  TEST_EQUAL(std::string("Ростов-на-Дону"), getter("Russia_Rostov-on-Don"), ());
  TEST_EQUAL(std::string("Russia_Murmansk"), getter("Russia_Murmansk"), ());
}
