#include "testing/testing.hpp"

#include "partners_api/taxi_places.hpp"
#include "partners_api/taxi_places_loader.hpp"
#include "partners_api/taxi_provider.hpp"

#include <vector>

namespace taxi
{
class PlacesTest
{
public:
  static void TestIntersections()
  {
    using places::Loader;

    std::vector<taxi::SupportedPlaces> places = {Loader::LoadFor(Provider::Type::Maxim),
                                                 Loader::LoadFor(Provider::Type::Uber),
                                                 Loader::LoadFor(Provider::Type::Yandex)};

    for (size_t i = 0; i < places.size(); ++i)
    {
      for (size_t j = 0; j < places.size(); ++j)
      {
        if (i == j)
          continue;

        FindIdenticalPlaces(places[i].m_enabledPlaces, places[j].m_enabledPlaces);
        FindIdenticalPlaces(places[i].m_disabledPlaces, places[j].m_disabledPlaces);
      }
    }
  }

private:
  static void FindIdenticalPlaces(taxi::Places const & lhs, taxi::Places const & rhs)
  {
    for (auto const & country : lhs.m_countries)
    {
      if (country.m_cities.empty())
      {
        auto const countryIt =
            std::find_if(rhs.m_countries.cbegin(), rhs.m_countries.cend(),
                         [&country](Places::Country const & c) { return c.m_id == country.m_id; });

        TEST(countryIt == rhs.m_countries.cend(), (*countryIt));
      }
      else
      {
        for (auto const & city : country.m_cities)
        {
          TEST(!rhs.Has(country.m_id, city), (country.m_id, city));
        }
      }
    }

    for (auto it = lhs.m_mwmIds.begin(); it != lhs.m_mwmIds.end(); ++it)
    {
      TEST(!rhs.Has(*it), (*it));
    }
  }
};
}  // namespace taxi

namespace
{
UNIT_TEST(TaxiPlaces_Intersections) { taxi::PlacesTest::TestIntersections(); }
}  // namespace
