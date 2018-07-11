#include "testing/testing.hpp"

#include "geocoder/geocoder.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"

#include <string>
#include <vector>

using namespace std;

namespace
{
double const kCertaintyEps = 1e-6;
}  // namespace

namespace geocoder
{
void TestGeocoder(Geocoder const & geocoder, string const & query, vector<Result> const & expected)
{
  vector<Result> actual;
  geocoder.ProcessQuery(query, actual);
  TEST_EQUAL(actual.size(), expected.size(), ());
  for (size_t i = 0; i < actual.size(); ++i)
  {
    TEST_EQUAL(actual[i].m_osmId, expected[i].m_osmId, ());
    TEST(my::AlmostEqualAbs(actual[i].m_certainty, expected[i].m_certainty, kCertaintyEps), ());
  }
}

UNIT_TEST(Geocoder_Smoke)
{
  Geocoder geocoder("" /* pathToJsonHierarchy */);

  TestGeocoder(geocoder, "a", {{osm::Id(10), 0.5}, {osm::Id(11), 1.0}});
  TestGeocoder(geocoder, "b", {{osm::Id(20), 0.8}, {osm::Id(21), 0.1}});
}
}  // namespace geocoder
