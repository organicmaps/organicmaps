#include "testing/testing.hpp"

#include "geocoder/geocoder.hpp"

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

  TestGeocoder(geocoder, "a",
               {{osm::Id(0xC00000000026FCFDULL), 0.5}, {osm::Id(0x40000000C4D63818ULL), 1.0}});
  TestGeocoder(geocoder, "b",
               {{osm::Id(0x8000000014527125ULL), 0.8}, {osm::Id(0x40000000F26943B9ULL), 0.1}});
}
}  // namespace geocoder
