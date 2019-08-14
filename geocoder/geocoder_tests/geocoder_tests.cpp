#include "testing/testing.hpp"

#include "geocoder/geocoder.hpp"
#include "geocoder/hierarchy_reader.hpp"

#include "indexer/search_string_utils.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include "base/geo_object_id.hpp"
#include "base/math.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <iomanip>
#include <string>
#include <vector>

using namespace platform::tests_support;
using namespace std;

namespace
{
using Id = base::GeoObjectId;

double const kCertaintyEps = 1e-6;
string const kRegionsData = R"#(
C00000000004B279 {"type": "Feature", "geometry": {"type": "Point", "coordinates": [-80.1142033187951, 21.55511095]}, "properties": {"locales": {"default": {"name": "Cuba", "address": {"country": "Cuba"}}}, "rank": 2}}
C0000000001C4CA7 {"type": "Feature", "geometry": {"type": "Point", "coordinates": [-78.7260117405499, 21.74300205]}, "properties": {"locales": {"default": {"name": "Ciego de Ávila", "address": {"region": "Ciego de Ávila", "country": "Cuba"}}}, "rank": 4}}
C00000000059D6B5 {"type": "Feature", "geometry": {"type": "Point", "coordinates": [-78.9263054493181, 22.08185765]}, "properties": {"locales": {"default": {"name": "Florencia", "address": {"subregion": "Florencia", "region": "Ciego de Ávila", "country": "Cuba"}}}, "rank": 6}}
)#";
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
  auto const & hierarchy = geocoder.GetHierarchy();
  auto const & dictionary = hierarchy.GetNormalizedNameDictionary();

  vector<Hierarchy::Entry> entries;
  geocoder.GetIndex().ForEachDocId({("florencia")}, [&](Index::DocId const & docId) {
    entries.emplace_back(geocoder.GetIndex().GetDoc(docId));
  });

  TEST_EQUAL(entries.size(), 1, ());
  TEST_EQUAL(entries[0].GetNormalizedMultipleNames(Type::Country, dictionary).GetMainName(), "cuba",
             ());
  TEST_EQUAL(entries[0].GetNormalizedMultipleNames(Type::Region, dictionary).GetMainName(),
             "ciego de avila", ());
  TEST_EQUAL(entries[0].GetNormalizedMultipleNames(Type::Subregion, dictionary).GetMainName(),
             "florencia", ());
}

UNIT_TEST(Geocoder_EnglishNames)
{
  string const kData = R"#(
10 {"properties": {"locales": {"default": {"address": {"locality": "Москва"}}, "en": {"address": {"locality": "Moscow"}}}}}
11 {"properties": {"locales": {"default": {"address": {"locality": "Москва", "street": "улица Новый Арбат"}}, "en": {"address": {"locality": "Moscow", "street": "New Arbat Avenue"}}}}}
)#";

  ScopedFile const regionsJsonFile("regions.jsonl", kData);
  Geocoder geocoder(regionsJsonFile.GetFullPath());

  TestGeocoder(geocoder, "Moscow, New Arbat", {{Id{0x11}, 1.0}, {Id{0x10}, 0.6}});
}

UNIT_TEST(Geocoder_OnlyBuildings)
{
  string const kData = R"#(
10 {"properties": {"locales": {"default": {"address": {"locality": "Some Locality"}}}}}

21 {"properties": {"locales": {"default": {"address": {"street": "Good", "locality": "Some Locality"}}}}}
22 {"properties": {"locales": {"default": {"address": {"building": "5", "street": "Good", "locality": "Some Locality"}}}}}

31 {"properties": {"locales": {"default": {"address": {"street": "Bad", "locality": "Some Locality"}}}}}
32 {"properties": {"locales": {"default": {"address": {"building": "10", "street": "Bad", "locality": "Some Locality"}}}}}

40 {"properties": {"locales": {"default": {"address": {"street": "MaybeNumbered", "locality": "Some Locality"}}}}}
41 {"properties": {"locales": {"default": {"address": {"street": "MaybeNumbered-3", "locality": "Some Locality"}}}}}
42 {"properties": {"locales": {"default": {"address": {"building": "3", "street": "MaybeNumbered", "locality": "Some Locality"}}}}}
)#";

  ScopedFile const regionsJsonFile("regions.jsonl", kData);
  Geocoder geocoder(regionsJsonFile.GetFullPath());

  base::GeoObjectId const localityId(0x10);
  base::GeoObjectId const goodStreetId(0x21);
  base::GeoObjectId const badStreetId(0x31);
  base::GeoObjectId const building5(0x22);
  base::GeoObjectId const building10(0x32);

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
  base::GeoObjectId const numberedStreet(0x41);
  base::GeoObjectId const houseOnANonNumberedStreet(0x42);
  TestGeocoder(geocoder, "some locality maybenumbered 3",
               {{numberedStreet, 1.0}, {houseOnANonNumberedStreet, 0.8875}});
}

UNIT_TEST(Geocoder_MismatchedLocality)
{
  string const kData = R"#(
10 {"properties": {"locales": {"default": {"address": {"locality": "Moscow"}}}}}
11 {"properties": {"locales": {"default": {"address": {"locality": "Paris"}}}}}

21 {"properties": {"locales": {"default": {"address": {"street": "Krymskaya", "locality": "Moscow"}}}}}
22 {"properties": {"locales": {"default": {"address": {"building": "2", "street": "Krymskaya", "locality": "Moscow"}}}}}

31 {"properties": {"locales": {"default": {"address": {"street": "Krymskaya", "locality": "Paris"}}}}}
32 {"properties": {"locales": {"default": {"address": {"building": "3", "street": "Krymskaya", "locality": "Paris"}}}}}
)#";

  ScopedFile const regionsJsonFile("regions.jsonl", kData);
  Geocoder geocoder(regionsJsonFile.GetFullPath());

  base::GeoObjectId const building2(0x22);

  TestGeocoder(geocoder, "Moscow Krymskaya 2", {{building2, 1.0}});

  // "Krymskaya 3" looks almost like a match to "Paris-Krymskaya-3" but we should not emit it.
  TestGeocoder(geocoder, "Moscow Krymskaya 3", {});
}

// Geocoder_StreetWithNumber* ----------------------------------------------------------------------
UNIT_TEST(Geocoder_StreetWithNumberInCity)
{
  string const kData = R"#(
10 {"properties": {"locales": {"default": {"address": {"locality": "Москва"}}}}}
11 {"properties": {"locales": {"default": {"address": {"locality": "Москва", "street": "улица 1905 года"}}}}}

20 {"properties": {"locales": {"default": {"address": {"locality": "Краснокамск"}}}}}
28 {"properties": {"locales": {"default": {"address": {"locality": "Краснокамск", "street": "улица 1905 года"}}}}}
)#";

  ScopedFile const regionsJsonFile("regions.jsonl", kData);
  Geocoder geocoder(regionsJsonFile.GetFullPath());

  TestGeocoder(geocoder, "Москва, улица 1905 года", {{Id{0x11}, 1.0}});
}

UNIT_TEST(Geocoder_StreetWithNumberInClassifiedCity)
{
  string const kData = R"#(
10 {"properties": {"locales": {"default": {"address": {"locality": "Москва"}}}}}
11 {"properties": {"locales": {"default": {"address": {"locality": "Москва", "street": "улица 1905 года"}}}}}
)#";

  ScopedFile const regionsJsonFile("regions.jsonl", kData);
  Geocoder geocoder(regionsJsonFile.GetFullPath());

  TestGeocoder(geocoder, "город Москва, улица 1905 года", {{Id{0x11}, 1.0}});
}

UNIT_TEST(Geocoder_StreetWithNumberInAnyCity)
{
  string const kData = R"#(
10 {"properties": {"locales": {"default": {"address": {"locality": "Москва"}}}}}
11 {"properties": {"locales": {"default": {"address": {"locality": "Москва", "street": "улица 1905 года"}}}}}

20 {"properties": {"locales": {"default": {"address": {"locality": "Краснокамск"}}}}}
28 {"properties": {"locales": {"default": {"address": {"locality": "Краснокамск", "street": "улица 1905 года"}}}}}
)#";

  ScopedFile const regionsJsonFile("regions.jsonl", kData);
  Geocoder geocoder(regionsJsonFile.GetFullPath());

  TestGeocoder(geocoder, "улица 1905 года", {{Id{0x11}, 1.0}, {Id{0x28}, 1.0}});
}

UNIT_TEST(Geocoder_StreetWithNumberAndWithoutStreetSynonym)
{
  string const kData = R"#(
10 {"properties": {"locales": {"default": {"address": {"locality": "Москва"}}}}}
11 {"properties": {"locales": {"default": {"address": {"locality": "Москва", "street": "улица 1905 года"}}}}}
)#";

  ScopedFile const regionsJsonFile("regions.jsonl", kData);
  Geocoder geocoder(regionsJsonFile.GetFullPath());

  TestGeocoder(geocoder, "Москва, 1905 года", {{Id{0x11}, 1.0}});
}

UNIT_TEST(Geocoder_UntypedStreetWithNumberAndStreetSynonym)
{
  string const kData = R"#(
10 {"properties": {"locales": {"default": {"address": {"locality": "Москва"}}}}}
13 {"properties": {"locales": {"default": {"address": {"locality": "Москва", "street": "8 Марта"}}}}}
)#";

  ScopedFile const regionsJsonFile("regions.jsonl", kData);
  Geocoder geocoder(regionsJsonFile.GetFullPath());

  TestGeocoder(geocoder, "Москва, улица 8 Марта", {{Id{0x13}, 1.0}});
}

UNIT_TEST(Geocoder_StreetWithTwoNumbers)
{
  string const kData = R"#(
10 {"properties": {"locales": {"default": {"address": {"locality": "Москва"}}}}}
12 {"properties": {"locales": {"default": {"address": {"locality": "Москва", "street": "4-я улица 8 Марта"}}}}}

13 {"properties": {"locales": {"default": {"address": {"locality": "Москва", "street": "улица 8 Марта"}}}}}
)#";

  ScopedFile const regionsJsonFile("regions.jsonl", kData);
  Geocoder geocoder(regionsJsonFile.GetFullPath());

  TestGeocoder(geocoder, "Москва, 4-я улица 8 Марта", {{Id{0x12}, 1.0}});
}

UNIT_TEST(Geocoder_BuildingOnStreetWithNumber)
{
  string const kData = R"#(
10 {"properties": {"locales": {"default": {"address": {"locality": "Москва"}}}}}
13 {"properties": {"locales": {"default": {"address": {"locality": "Москва", "street": "улица 8 Марта"}}}}}
15 {"properties": {"locales": {"default": {"address": {"locality": "Москва", "street": "улица 8 Марта", "building": "4"}}}}}
)#";

  ScopedFile const regionsJsonFile("regions.jsonl", kData);
  Geocoder geocoder(regionsJsonFile.GetFullPath());

  TestGeocoder(geocoder, "Москва, улица 8 Марта, 4", {{Id{0x15}, 1.0}});
}

//--------------------------------------------------------------------------------------------------
UNIT_TEST(Geocoder_LocalityBuilding)
{
  string const kData = R"#(
10 {"properties": {"locales": {"default": {"address": {"locality": "Zelenograd"}}}}}
22 {"properties": {"locales": {"default": {"address": {"building": "2", "locality": "Zelenograd"}}}}}
31 {"properties": {"locales": {"default": {"address": {"street": "Krymskaya", "locality": "Zelenograd"}}}}}
32 {"properties": {"locales": {"default": {"address": {"building": "2", "street": "Krymskaya", "locality": "Zelenograd"}}}}}
)#";
  ScopedFile const regionsJsonFile("regions.jsonl", kData);
  Geocoder geocoder(regionsJsonFile.GetFullPath());
  base::GeoObjectId const building2(0x22);
  TestGeocoder(geocoder, "Zelenograd 2", {{building2, 1.0}});
}

// Geocoder_Subregion* -----------------------------------------------------------------------------
UNIT_TEST(Geocoder_SubregionInLocality)
{
  string const kData = R"#(
10 {"properties": {"locales": {"default": {"address": {"region": "Москва"}}}, "rank": 2}}
11 {"properties": {"locales": {"default": {"address": {"locality": "Москва", "region": "Москва"}}}, "rank": 4}}
12 {"properties": {"locales": {"default": {"address": {"subregion": "Северный административный округ", "locality": "Москва", "region": "Москва"}}}, "rank": 3}}
)#";

  ScopedFile const regionsJsonFile("regions.jsonl", kData);
  Geocoder geocoder(regionsJsonFile.GetFullPath());

  TestGeocoder(geocoder, "Северный административный округ", {{Id{0x12}, 1.0}});
  TestGeocoder(geocoder, "Москва, Северный административный округ",
               {{Id{0x12}, 1.0}, {Id{0x10}, 0.294118}, {Id{0x11}, 0.176471}});
  TestGeocoder(geocoder, "Москва", {{Id{0x10}, 1.0}, {Id{0x11}, 0.6}});
}

// Geocoder_NumericalSuburb* ----------------------------------------------------------------------
UNIT_TEST(Geocoder_NumericalSuburbRelevance)
{
  string const kData = R"#(
10 {"properties": {"locales": {"default": {"address": {"region": "Metro Manila"}}}}}
11 {"properties": {"locales": {"default": {"address": {"locality": "Caloocan", "region": "Metro Manila"}}}}}
12 {"properties": {"locales": {"default": {"address": {"suburb": "60", "locality": "Caloocan", "region": "Metro Manila"}}}}}
20 {"properties": {"locales": {"default": {"address": {"locality": "Белгород"}}}}}
21 {"properties": {"locales": {"default": {"address": {"street": "Щорса", "locality": "Белгород"}}}}}
22 {"properties": {"locales": {"default": {"address": {"building": "60", "street": "Щорса", "locality": "Белгород"}}}}}
)#";

  ScopedFile const regionsJsonFile("regions.jsonl", kData);
  Geocoder geocoder(regionsJsonFile.GetFullPath());

  TestGeocoder(geocoder, "Caloocan, 60", {{Id{0x12}, 1.0}});
  TestGeocoder(geocoder, "60", {});
  TestGeocoder(geocoder, "Metro Manila, 60", {{Id{0x10}, 1.0}});
  TestGeocoder(geocoder, "Белгород, Щорса, 60", {{Id{0x22}, 1.0}});
}

//--------------------------------------------------------------------------------------------------
UNIT_TEST(Geocoder_EmptyFileConcurrentRead)
{
  ScopedFile const regionsJsonFile("regions.jsonl", "");
  Geocoder geocoder(regionsJsonFile.GetFullPath(), 8 /* reader threads */);

  TEST_EQUAL(geocoder.GetHierarchy().GetEntries().size(), 0, ());
}

UNIT_TEST(Geocoder_BigFileConcurrentRead)
{
  int const kEntryCount = 100000;

  stringstream s;
  for (int i = 0; i < kEntryCount; ++i)
  {
    s << setw(16) << setfill('0') << hex << uppercase << i << " "
      << "{"
      << R"("type": "Feature",)"
      << R"("geometry": {"type": "Point", "coordinates": [0, 0]},)"
      << R"("properties": {"locales": {"default": {)"
      << R"("name": ")" << i << R"(", "address": {"country": ")" << i << R"("}}}, "rank": 2})"
      << "}\n";
  }

  ScopedFile const regionsJsonFile("regions.jsonl", s.str());
  Geocoder geocoder(regionsJsonFile.GetFullPath(), 8 /* reader threads */);

  TEST_EQUAL(geocoder.GetHierarchy().GetEntries().size(), kEntryCount, ());
}
}  // namespace geocoder
