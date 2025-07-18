#include "testing/testing.hpp"

#include "search/region_info_getter.hpp"

#include <string>

using namespace search;

namespace
{
class RegionInfoGetterTest
{
public:
  RegionInfoGetterTest()
  {
    m_regionInfoGetter.LoadCountriesTree();
    SetLocale("default");
  }

  void SetLocale(std::string const & locale) { m_regionInfoGetter.SetLocale(locale); }

  std::string GetLocalizedFullName(storage::CountryId const & id) const
  {
    return m_regionInfoGetter.GetLocalizedFullName(id);
  }

  std::string GetLocalizedCountryName(storage::CountryId const & id) const
  {
    return m_regionInfoGetter.GetLocalizedCountryName(id);
  }

protected:
  RegionInfoGetter m_regionInfoGetter;
};

UNIT_CLASS_TEST(RegionInfoGetterTest, CountryName)
{
  SetLocale("en");
  TEST_EQUAL(GetLocalizedCountryName("Russia_Moscow Oblast_East"), "Moscow Oblast", ());
  TEST_EQUAL(GetLocalizedCountryName("Russia_Moscow"), "Moscow", ());
  TEST_EQUAL(GetLocalizedCountryName("United States of America"), "USA", ());

  SetLocale("ru");
  TEST_EQUAL(GetLocalizedCountryName("Russia_Moscow Oblast_East"), "Московская область", ());
  TEST_EQUAL(GetLocalizedCountryName("Russia_Moscow"), "Москва", ());
  TEST_EQUAL(GetLocalizedCountryName("United States of America"), "США", ());
  TEST_EQUAL(GetLocalizedCountryName("Crimea"), "Крым", ());

  // En locale should be actually used.
  SetLocale("broken locale");
  TEST_EQUAL(GetLocalizedCountryName("Russia_Moscow Oblast_East"), "Moscow Oblast", ());
  TEST_EQUAL(GetLocalizedCountryName("Russia_Moscow"), "Moscow", ());
  TEST_EQUAL(GetLocalizedCountryName("United States of America"), "USA", ());

  SetLocale("zh-Hans");
  TEST_EQUAL(GetLocalizedCountryName("Russia_Moscow Oblast_East"), "莫斯科州", ());
  TEST_EQUAL(GetLocalizedCountryName("Russia_Moscow"), "莫斯科", ());
  TEST_EQUAL(GetLocalizedCountryName("United States of America"), "美国", ());
}

UNIT_CLASS_TEST(RegionInfoGetterTest, FullName)
{
  SetLocale("ru");
  TEST_EQUAL(GetLocalizedFullName("Russia_Moscow Oblast_East"), "Московская область, Россия", ());
  TEST_EQUAL(GetLocalizedFullName("Crimea"), "Крым", ());
}
}  // namespace
