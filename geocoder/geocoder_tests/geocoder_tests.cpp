#include "testing/testing.hpp"

#include "geocoder/geocoder.hpp"

#include "indexer/search_string_utils.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include "base/geo_object_id.hpp"
#include "base/math.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <string>
#include <vector>

using namespace platform::tests_support;
using namespace std;

namespace
{
double const kCertaintyEps = 1e-6;

string const kRegionsData = R"#(
-4611686018427080071 {"type": "Feature", "geometry": {"type": "Point", "coordinates": [-80.1142033187951, 21.55511095]}, "properties": {"name": "Cuba", "rank": 2, "address": {"country": "Cuba"}}}
-4611686018425533273 {"type": "Feature", "geometry": {"type": "Point", "coordinates": [-78.7260117405499, 21.74300205]}, "properties": {"name": "Ciego de Ávila", "rank": 4, "address": {"region": "Ciego de Ávila", "country": "Cuba"}}}
-4611686018421500235 {"type": "Feature", "geometry": {"type": "Point", "coordinates": [-78.9263054493181, 22.08185765]}, "properties": {"name": "Florencia", "rank": 6, "address": {"subregion": "Florencia", "region": "Ciego de Ávila", "country": "Cuba"}}}
)#";

geocoder::Tokens Split(string const & s)
{
  geocoder::Tokens result;
  search::NormalizeAndTokenizeAsUtf8(s, result);
  return result;
}
}  // namespace

namespace geocoder
{
void TestGeocoder(Geocoder & geocoder, string const & query, vector<Result> && expected)
{
  vector<Result> actual;
  geocoder.ProcessQuery(query, actual);
  TEST_EQUAL(actual.size(), expected.size(), (query, actual, expected));
  sort(actual.begin(), actual.end(), base::LessBy(&Result::m_osmId));
  sort(expected.begin(), expected.end(), base::LessBy(&Result::m_osmId));
  for (size_t i = 0; i < actual.size(); ++i)
  {
    TEST(actual[i].m_certainty >= 0.0 && actual[i].m_certainty <= 1.0,
         (query, actual[i].m_certainty));
    TEST_EQUAL(actual[i].m_osmId, expected[i].m_osmId, (query));
    TEST(base::AlmostEqualAbs(actual[i].m_certainty, expected[i].m_certainty, kCertaintyEps),
         (query, actual[i].m_certainty, expected[i].m_certainty));
  }
}

UNIT_TEST(Geocoder_Smoke)
{
  ScopedFile const regionsJsonFile("regions.jsonl", kRegionsData);
  Geocoder geocoder(regionsJsonFile.GetFullPath());

  base::GeoObjectId const florenciaId(0xc00000000059d6b5);
  base::GeoObjectId const cubaId(0xc00000000004b279);

  TestGeocoder(geocoder, "florencia", {{florenciaId, 1.0}});
  TestGeocoder(geocoder, "cuba florencia", {{florenciaId, 1.0}, {cubaId, 0.714286}});
  TestGeocoder(geocoder, "florencia somewhere in cuba", {{cubaId, 0.714286}, {florenciaId, 1.0}});
}

UNIT_TEST(Geocoder_Hierarchy)
{
  ScopedFile const regionsJsonFile("regions.jsonl", kRegionsData);
  Geocoder geocoder(regionsJsonFile.GetFullPath());

  auto entries = geocoder.GetHierarchy().GetEntries({("florencia")});

  TEST(entries, ());
  TEST_EQUAL(entries->size(), 1, ());
  TEST_EQUAL((*entries)[0]->m_address[static_cast<size_t>(Type::Country)], Split("cuba"), ());
  TEST_EQUAL((*entries)[0]->m_address[static_cast<size_t>(Type::Region)], Split("ciego de avila"),
             ());
  TEST_EQUAL((*entries)[0]->m_address[static_cast<size_t>(Type::Subregion)], Split("florencia"),
             ());
}

UNIT_TEST(Geocoder_OnlyBuildings)
{
  string const kData = R"#(
10 {"properties": {"address": {"locality": "Some Locality"}}}

21 {"properties": {"address": {"street": "Good", "locality": "Some Locality"}}}
22 {"properties": {"address": {"building": "5", "street": "Good", "locality": "Some Locality"}}}

31 {"properties": {"address": {"street": "Bad", "locality": "Some Locality"}}}
32 {"properties": {"address": {"building": "10", "street": "Bad", "locality": "Some Locality"}}}

40 {"properties": {"address": {"street": "MaybeNumbered", "locality": "Some Locality"}}}
41 {"properties": {"address": {"street": "MaybeNumbered-3", "locality": "Some Locality"}}}
42 {"properties": {"address": {"building": "3", "street": "MaybeNumbered", "locality": "Some Locality"}}}
)#";

  ScopedFile const regionsJsonFile("regions.jsonl", kData);
  Geocoder geocoder(regionsJsonFile.GetFullPath());

  base::GeoObjectId const localityId(10);
  base::GeoObjectId const goodStreetId(21);
  base::GeoObjectId const badStreetId(31);
  base::GeoObjectId const building5(22);
  base::GeoObjectId const building10(32);

  TestGeocoder(geocoder, "some locality", {{localityId, 1.0}});
  TestGeocoder(geocoder, "some locality good", {{goodStreetId, 1.0}, {localityId, 0.857143}});
  TestGeocoder(geocoder, "some locality bad", {{badStreetId, 1.0}, {localityId, 0.857143}});

  TestGeocoder(geocoder, "some locality good 5", {{building5, 1.0}});
  TestGeocoder(geocoder, "some locality bad 10", {{building10, 1.0}});

  // There is a building "10" on Bad Street but we should not return it.
  // Another possible resolution would be to return just "Good Street" (relaxed matching)
  // but at the time of writing the goal is to either have an exact match or no match at all.
  TestGeocoder(geocoder, "some locality good 10", {});

  // Sometimes we may still emit a non-building.
  // In this case it happens because all query tokens are used.
  base::GeoObjectId const numberedStreet(41);
  base::GeoObjectId const houseOnANonNumberedStreet(42);
  TestGeocoder(geocoder, "some locality maybenumbered 3",
               {{numberedStreet, 1.0}, {houseOnANonNumberedStreet, 0.8875}});
}

UNIT_TEST(Geocoder_MismatchedLocality)
{
  string const kData = R"#(
10 {"properties": {"address": {"locality": "Moscow"}}}
11 {"properties": {"address": {"locality": "Paris"}}}

21 {"properties": {"address": {"street": "Street", "locality": "Moscow"}}}
22 {"properties": {"address": {"building": "2", "street": "Street", "locality": "Moscow"}}}

31 {"properties": {"address": {"street": "Street", "locality": "Paris"}}}
32 {"properties": {"address": {"building": "3", "street": "Street", "locality": "Paris"}}}
)#";

  ScopedFile const regionsJsonFile("regions.jsonl", kData);
  Geocoder geocoder(regionsJsonFile.GetFullPath());

  base::GeoObjectId const building2(22);

  TestGeocoder(geocoder, "Moscow Street 2", {{building2, 1.0}});

  // "Street 3" looks almost like a match to "Paris-Street-3" but we should not emit it.
  TestGeocoder(geocoder, "Moscow Street 3", {});
}
}  // namespace geocoder
