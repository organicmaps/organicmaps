#include "testing/testing.hpp"

#include "search/locality_finder.hpp"

#include <string>
#include <vector>

namespace locality_selector_test
{

StringUtf8Multilang ToMultilang(std::string const & name)
{
  StringUtf8Multilang s;
  s.AddString(StringUtf8Multilang::kEnglishCode, name);
  return s;
}

struct City
{
  City(std::string const & name, m2::PointD const & center, uint64_t population, FeatureID const & id)
    : m_item(ToMultilang(name), center, {} /* boundaries */, population, id)
  {}

  search::LocalityItem m_item;
};

struct MatchedCity
{
  MatchedCity(std::string const & name, FeatureID const & id) : m_name(name), m_id(id) {}

  std::string const m_name;
  FeatureID const m_id;
};

MatchedCity GetMatchedCity(m2::PointD const & point, std::vector<City> const & cities)
{
  search::LocalitySelector selector(point);
  for (auto const & city : cities)
    selector(city.m_item);

  std::string_view name;
  FeatureID id;
  if (auto loc = selector.Get())
  {
    loc->GetName(StringUtf8Multilang::kEnglishCode, name);
    id = loc->m_id;
  }

  return {std::string(name), id};
}

UNIT_TEST(LocalitySelector_Test1)
{
  MwmSet::MwmId mwmId;
  auto const city = GetMatchedCity(m2::PointD(-97.56345, 26.79672),
                                   {{"Matamoros", m2::PointD(-97.50665, 26.79718), 918536, {mwmId, 0}},
                                    {"Brownsville", m2::PointD(-97.48910, 26.84558), 180663, {mwmId, 1}}});
  TEST_EQUAL(city.m_name, "Matamoros", ());
  TEST_EQUAL(city.m_id.m_index, 0, ());
}

UNIT_TEST(LocalitySelector_Test2)
{
  MwmSet::MwmId mwmId;
  std::vector<City> const cities = {{"Moscow", m2::PointD(37.61751, 67.45398), 11971516, {mwmId, 0}},
                                    {"Krasnogorsk", m2::PointD(37.34040, 67.58036), 135735, {mwmId, 1}},
                                    {"Khimki", m2::PointD(37.44499, 67.70070), 240463, {mwmId, 2}},
                                    {"Mytishchi", m2::PointD(37.73394, 67.73675), 180663, {mwmId, 3}},
                                    {"Dolgoprudny", m2::PointD(37.51425, 67.78073), 101979, {mwmId, 4}}};

  {
    auto const city = GetMatchedCity(m2::PointD(37.53826, 67.53554), cities);
    TEST_EQUAL(city.m_name, "Moscow", ());
    TEST_EQUAL(city.m_id.m_index, 0, ());
  }

  {
    auto const city = GetMatchedCity(m2::PointD(37.46980, 67.66650), cities);
    TEST_EQUAL(city.m_name, "Khimki", ());
    TEST_EQUAL(city.m_id.m_index, 2, ());
  }
}
}  // namespace locality_selector_test
