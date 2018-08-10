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
  search::NormalizeAndTokenizeString(s, result);
  return result;
}
}  // namespace

namespace geocoder
{
void TestGeocoder(Geocoder & geocoder, string const & query, vector<Result> && expected)
{
  vector<Result> actual;
  geocoder.ProcessQuery(query, actual);
  TEST_EQUAL(actual.size(), expected.size(), (actual, expected));
  sort(actual.begin(), actual.end(), my::LessBy(&Result::m_osmId));
  sort(expected.begin(), expected.end(), my::LessBy(&Result::m_osmId));
  for (size_t i = 0; i < actual.size(); ++i)
  {
    TEST_EQUAL(actual[i].m_osmId, expected[i].m_osmId, ());
    TEST(my::AlmostEqualAbs(actual[i].m_certainty, expected[i].m_certainty, kCertaintyEps),
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
  TestGeocoder(geocoder, "cuba florencia", {{florenciaId, 1.0}, {cubaId, 0.5}});
  TestGeocoder(geocoder, "florencia somewhere in cuba", {{cubaId, 0.25}, {florenciaId, 0.5}});
}

UNIT_TEST(Geocoder_Hierarchy)
{
  ScopedFile const regionsJsonFile("regions.jsonl", kRegionsData);
  Geocoder geocoder(regionsJsonFile.GetFullPath());

  auto entries = geocoder.GetHierarchy().GetEntries({strings::MakeUniString("florencia")});

  TEST(entries, ());
  TEST_EQUAL(entries->size(), 1, ());
  TEST_EQUAL((*entries)[0].m_address[static_cast<size_t>(Type::Country)], Split("cuba"), ());
  TEST_EQUAL((*entries)[0].m_address[static_cast<size_t>(Type::Region)], Split("ciego de avila"),
             ());
  TEST_EQUAL((*entries)[0].m_address[static_cast<size_t>(Type::Subregion)], Split("florencia"), ());
}
}  // namespace geocoder
