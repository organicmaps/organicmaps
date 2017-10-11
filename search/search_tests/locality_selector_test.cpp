#include "testing/testing.hpp"

#include "search/locality_finder.hpp"

#include "base/string_utils.hpp"

using namespace search;

namespace
{
StringUtf8Multilang ToMultilang(string const & name)
{
  StringUtf8Multilang s;
  s.AddString(StringUtf8Multilang::kEnglishCode, name);
  return s;
}

struct City
{
  City(string const & name, m2::PointD const & center, uint64_t population)
    : m_item(ToMultilang(name), center, population)
  {
  }

  LocalityItem m_item;
};

string GetMatchedCity(m2::PointD const & point, vector<City> const & cities)
{
  LocalitySelector selector(point);
  for (auto const & city : cities)
    selector(city.m_item);

  string name;
  selector.WithBestLocality(
      [&](LocalityItem const & item) { item.GetName(StringUtf8Multilang::kEnglishCode, name); });
  return name;
}

// TODO (@y): this test fails for now. Need to uncomment it as soon as
// locality finder will be fixed.
//
// UNIT_TEST(LocalitySelector_Test1)
// {
//   auto const name = GetMatchedCity(
//       m2::PointD(-97.56345, 26.79672),
//       {{"Matamoros", m2::PointD(-97.50665, 26.79718), 10000},

//        {"Brownsville", m2::PointD(-97.48910, 26.84558), 180663}});
//   TEST_EQUAL(name, "Matamoros", ());
// }

UNIT_TEST(LocalitySelector_Test2)
{
  vector<City> const cities = {{"Moscow", m2::PointD(37.61751, 67.45398), 11971516},
                               {"Krasnogorsk", m2::PointD(37.34040, 67.58036), 135735},
                               {"Khimki", m2::PointD(37.44499, 67.70070), 240463},
                               {"Mytishchi", m2::PointD(37.73394, 67.73675), 180663},
                               {"Dolgoprudny", m2::PointD(37.51425, 67.78073), 101979}};

  {
    auto const name = GetMatchedCity(m2::PointD(37.53826, 67.53554), cities);
    TEST_EQUAL(name, "Moscow", ());
  }

  {
    auto const name = GetMatchedCity(m2::PointD(37.46980, 67.66650), cities);
    TEST_EQUAL(name, "Khimki", ());
  }
}
}  // namespace
