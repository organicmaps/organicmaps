#include "testing/testing.hpp"

#include "geocoder/geocoder.hpp"

#include "indexer/search_string_utils.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include "base/math.hpp"

#include <string>
#include <vector>

using namespace platform::tests_support;
using namespace std;

namespace
{
double const kCertaintyEps = 1e-6;

string const kRegionsData = R"#(
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

UNIT_TEST(Geocoder_Hierarchy)
{
  ScopedFile const regionsJsonFile("regions.jsonl", kRegionsData);
  Geocoder geocoder(regionsJsonFile.GetFullPath());

  auto entries = geocoder.GetHierarchy().GetEntries({strings::MakeUniString("florencia")});

  TEST(entries, ());
  TEST_EQUAL(entries->size(), 1, ());
  TEST_EQUAL((*entries)[0].m_address[static_cast<size_t>(Hierarchy::EntryType::Country)],
             Split("cuba"), ());
  TEST_EQUAL((*entries)[0].m_address[static_cast<size_t>(Hierarchy::EntryType::Region)],
             Split("ciego de avila"), ());
  TEST_EQUAL((*entries)[0].m_address[static_cast<size_t>(Hierarchy::EntryType::Subregion)],
             Split("florencia"), ());
}
}  // namespace geocoder
