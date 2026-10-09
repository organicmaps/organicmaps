#include "search/address_matcher.hpp"

#include "geometry/mercator.hpp"

#include "testing/testing.hpp"

#include <algorithm>
#include <string>

namespace address_matcher_tests
{
using namespace search;

Result MakeAddress(double lat, double lon, std::string const & name, std::string const & address = "Surrey, Canada")
{
  Result result(mercator::FromLatLon(lat, lon), name);
  result.SetAddress(std::string(address));
  result.SetType(Result::Type::LatLon);
  return result;
}

UNIT_TEST(AddressMatcher_PreservesExactAddress)
{
  Results source;
  source.AddResultNoChecks(MakeAddress(49.1208601, -122.8575937, "6486, 131A Street"));
  source.AddResultNoChecks(MakeAddress(49.1209318, -122.8577086, "6492, 131A Street"));
  source.AddResultNoChecks(MakeAddress(49.1210035, -122.8578235, "6498, 131A Street"));

  auto const results = RankAddressResults("6498 131A Street", source);
  TEST_EQUAL(results.GetCount(), 1, ());
  TEST_EQUAL(results[0].GetString(), "6498, 131A Street", ());
}

UNIT_TEST(AddressMatcher_PreservesDistinctExactAddresses)
{
  Results source;
  source.AddResultNoChecks(MakeAddress(0, 0, "121, Main Street", "Springfield, Illinois, USA"));
  source.AddResultNoChecks(MakeAddress(0, 1, "123, Main Street", "Springfield, Illinois, USA"));
  source.AddResultNoChecks(MakeAddress(0, 2, "123, Main Street", "Springfield, Massachusetts, USA"));
  auto const results = RankAddressResults("123 Main Street", source);
  TEST_EQUAL(results.GetCount(), 2, ());
  TEST_EQUAL(results[0].GetFeatureCenter(), source[1].GetFeatureCenter(), ());
  TEST_EQUAL(results[1].GetFeatureCenter(), source[2].GetFeatureCenter(), ());
  TEST(!FindUniqueAddressResult("123 Main Street", results), ());
  auto const * qualified = FindUniqueAddressResult("123 Main Street Springfield Illinois USA", results);
  TEST(qualified, ());
  TEST_EQUAL(qualified->GetFeatureCenter(), source[1].GetFeatureCenter(), ());
}

UNIT_TEST(AddressMatcher_NormalizesQueriesConsistently)
{
  auto const result = MakeAddress(49.87, -97.14, "578, Corydon Avenue", "Winnipeg, MB, Canada");
  for (auto const & query :
       {"578 Corydon Ave #10, Winnipeg, MB R3L 0P2, Canada", "Unit 10, 578 Corydon Ave, Winnipeg, MB R3L0P2, Canada"})
  {
    TEST(IsAddressQuery(query), (query));
    TEST(IsAddressResultMatchingQuery(query, result), (query));
    TEST_EQUAL(GetAddressResultMatch(query, result), AddressResultMatch::Exact, (query));
  }
  auto const suffixed = MakeAddress(0, 0, "12A, Main Street", "Surrey, Canada");
  TEST(IsAddressResultMatchingQuery("12a Main St, Surrey, Canada", suffixed), ());
  TEST(!IsAddressResultMatchingQuery("12 Main St, Surrey, Canada", suffixed), ());
  TEST(!IsAddressResultMatchingQuery("12B Main St, Surrey, Canada", suffixed), ());
  TEST(IsAddressResultMatchingQuery("12a Main St, Surrey, Canada",
                                    MakeAddress(0, 0, "Main Street, 12A", "Surrey, Canada")),
       ());
}

UNIT_TEST(AddressMatcher_ResolvesDuplicateResultsAtTheSameLocation)
{
  Results source;
  source.AddResultNoChecks(MakeAddress(0, 1, "123, Main Street"));
  source.AddResultNoChecks(MakeAddress(0, 1, "Main Street, 123"));
  auto const * result = FindUniqueAddressResult("123 Main Street", source);
  TEST(result, ());
  TEST_EQUAL(result->GetFeatureCenter(), source[0].GetFeatureCenter(), ());
  TEST(!ParseAddressQuery(std::string(4097, '1') + " Main Street"), ());
}

UNIT_TEST(AddressMatcher_InternationalHouseOrderAndPostcodes)
{
  struct Example
  {
    std::string m_query;
    std::string m_name;
    std::string m_context;
  };
  for (auto const & example : std::vector<Example>{
           {"Hauptstraße 12a, 10115 Berlin, Germany", "Hauptstraße, 12A", "Berlin, Germany"},
           {"12 Rue de Rivoli, 75001 Paris, France", "Rue de Rivoli, 12", "Paris, France"},
           {"Calle Mayor 12, 28013 Madrid, Spain", "Calle Mayor, 12", "Madrid, Spain"},
           {"Via Roma 12-14, 00100 Roma, Italy", "Via Roma, 12-14", "Roma, Italy"},
           {"Křižíkova 12/1, 18600 Praha, Czechia", "Křižíkova, 12/1", "Praha, Czech Republic"},
           {"улица Ленина 12, 123456 Москва, Россия", "улица Ленина, 12", "Москва, Россия"},
           {"10 Downing Street, London SW1A 2AA, UK", "Downing Street, 10", "London, United Kingdom"},
           {"2/14 Smith Street, Sydney NSW 2000, Australia", "Smith Street, 14", "Sydney, NSW, Australia"},
           {"12 MG Road, Bengaluru, 560001, India", "MG Road, 12", "Bengaluru, India"},
           {"Jingumae 1-2-3, Tokyo, 1500001, Japan", "Jingumae, 1-2-3", "Tokyo, Japan"},
           {"123 Main St, New York NY 10001, USA", "Main Street, 123", "New York, NY, USA"}})
  {
    auto const result = MakeAddress(0, 0, example.m_name, example.m_context);
    TEST(IsAddressResultMatchingQuery(example.m_query, result), (example.m_query));
    auto const wrong = MakeAddress(0, 1, example.m_name, "Othercity, Othercountry");
    TEST(!IsAddressResultMatchingQuery(example.m_query, wrong), (example.m_query));
  }
  for (auto const & query : {"12 Rue de Rivoli", "12a Rue de Rivoli", "12a Calle Mayor", "Calle Mayor 12",
                             "Hauptstraße 12a", "улица Ленина 12"})
    TEST(IsAddressQuery(query), (query));
  TEST(!IsAddressQuery("Studio 54"), ());
  TEST(!ParseAddressQuery("SW1A 2AA"), ());
  TEST(!ParseAddressQuery("Hauptstraße 999999999999999999999999"), ());
  TEST(!IsAddressResultMatchingQuery("12 MG Road, Sector 123, India",
                                     MakeAddress(0, 0, "MG Road, 12", "Sector 124, India")),
       ());
}

UNIT_TEST(AddressMatcher_CompoundHouseNumbersDoNotMatchNearbyNumbers)
{
  auto const result = MakeAddress(0, 0, "Via Roma, 12-14", "Roma, Italy");
  TEST(IsAddressResultMatchingQuery("Via Roma 12-14, Roma, Italy", result), ());
  TEST(!IsAddressResultMatchingQuery("Via Roma 12, Roma, Italy", result), ());
  TEST(!IsAddressResultMatchingQuery("Via Roma 12-16, Roma, Italy", result), ());
  TEST(!IsAddressResultMatchingQuery("Křižíkova 12/1, Praha, Czechia",
                                     MakeAddress(0, 0, "Křižíkova, 12", "Praha, Czechia")),
       ());
}

UNIT_TEST(AddressMatcher_NearbyRankingDoesNotRequireSameParity)
{
  Results source;
  source.AddResultNoChecks(MakeAddress(0, 0, "Starbucks"));
  source.AddResultNoChecks(MakeAddress(0, 0.0001, "6486, 131A Street"));
  source.AddResultNoChecks(MakeAddress(0, 0.0002, "6497, 131A Street"));
  auto const results = RankAddressResults("6498 131A Street", source);
  TEST_EQUAL(results.GetCount(), source.GetCount(), ());
  TEST_EQUAL(results[0].GetString(), "6497, 131A Street", ());
  TEST_EQUAL(GetAddressResultMatch("6498 131A Street", results[0]), AddressResultMatch::Nearby, ());
  TEST(!IsAddressResultMatchingQuery("6498 131A Street", results[0]), ());
}

UNIT_TEST(AddressMatcher_PreservesSingleNearbyAddress)
{
  Results source;
  source.AddResultNoChecks(MakeAddress(49.1209318, -122.8577086, "6492, 131A Street"));

  auto const results = RankAddressResults("6498 131A Street", source);
  TEST_EQUAL(results.GetCount(), 1, ());
  TEST_EQUAL(results[0].GetString(), "6492, 131A Street", ());
}

UNIT_TEST(AddressMatcher_PrioritizesMappedAddressesWithoutLocality)
{
  Results source;
  source.AddResultNoChecks(MakeAddress(49.12, -122.85, "Starbucks"));
  source.AddResultNoChecks(MakeAddress(49.1207, -122.8575, "6480, 131A Street"));
  source.AddResultNoChecks(MakeAddress(49.1209318, -122.8577086, "6492, 131A Street"));
  auto const results = RankAddressResults("6498 131a st", source);
  TEST_EQUAL(results.GetCount(), source.GetCount(), ());
  TEST_EQUAL(results[0].GetString(), "6492, 131A Street", ());
  TEST_EQUAL(results[1].GetString(), "6480, 131A Street", ());
  TEST_EQUAL(results[2].GetString(), "Starbucks", ());

  source.AddResultNoChecks(MakeAddress(49.121, -122.8578, "6498, 131A Street"));
  auto const exact = RankAddressResults("6498 131a st", source);
  TEST_EQUAL(exact.GetCount(), 2, ());
  TEST_EQUAL(exact[0].GetString(), "6498, 131A Street", ());
  TEST_EQUAL(exact[1].GetString(), "Starbucks", ());
}

UNIT_TEST(AddressMatcher_RequiresTheCompleteStreetName)
{
  Results source;
  source.AddResultNoChecks(MakeAddress(49.1207362, -122.8575047, "6480, Main Street"));
  source.AddResultNoChecks(MakeAddress(49.1208601, -122.8575937, "6486, Main Street"));

  auto const results = RankAddressResults("6490 Main Street West", source);
  TEST_EQUAL(results.GetCount(), 2, ());
}

UNIT_TEST(AddressMatcher_RejectsUnrelatedAddress)
{
  Results source;
  source.AddResultNoChecks(MakeAddress(49.20538, -122.770653, "16291, 111A Avenue"));

  auto const results = RankAddressResults("11378 158A Street Surrey", source);
  TEST_EQUAL(results.GetCount(), 1, ());
  TEST(!IsAddressResultMatchingQuery("11378 158A Street Surrey", results[0]), ());
}

UNIT_TEST(AddressMatcher_RequiresCompleteStreetIndependentlyOfLocality)
{
  auto const wrong = MakeAddress(0, 0, "123, Main Street West", "Springfield, Illinois, USA");
  TEST(!IsAddressResultMatchingQuery("123 Main Street Springfield Illinois USA", wrong, "123 Main Street"), ());
  auto const correct = MakeAddress(0, 0, "123, Main Street", "Springfield, Illinois, United States");
  TEST(IsAddressResultMatchingQuery("123 Main Street Springfield Illinois USA", correct, "123 Main Street"), ());
  TEST(!IsAddressResultMatchingQuery("123 Main Street Springfield Massachusetts USA", correct, "123 Main Street"), ());
  Results supports;
  supports.AddResultNoChecks(MakeAddress(0, 0, "120, Main Street", "West Springfield"));
  supports.AddResultNoChecks(MakeAddress(0, 0.0001, "122, Main Street", "West Springfield"));
  TEST(!FindUniqueAddressResult("124 Main Street West Springfield", supports, "124 Main Street West"), ());
}

UNIT_TEST(AddressMatcher_FormattedFrenchAndNumericOverflow)
{
  auto const result = MakeAddress(48.855, 2.36, "12, Rue de Rivoli", "Paris, France");
  TEST(IsAddressResultMatchingQuery("12 Rue de Rivoli Paris France", result, "12 Rue de Rivoli"), ());
  TEST(!IsAddressResultMatchingQuery("999999999999999999999999 Rue de Rivoli Paris France", result,
                                     "999999999999999999999999 Rue de Rivoli"),
       ());
}

UNIT_TEST(AddressMatcher_LocalityAbbreviationsRetainDisambiguation)
{
  auto const address = MakeAddress(0, 0, "8877, Wright Street", "Langley Township, British Columbia, Canada");
  for (auto const * locality : {"Langley Twp", "Langley TWP.", "Langley Twnshp", "Langley Township"})
    TEST(IsAddressResultMatchingQuery(std::string("8877 Wright St ") + locality + " British Columbia Canada", address),
         ());
  TEST(!IsAddressResultMatchingQuery("8877 Wright St Other Twp British Columbia Canada", address), ());
  auto const borough = MakeAddress(0, 0, "123, Main Street", "Queens Borough, New York");
  TEST(IsAddressResultMatchingQuery("123 Main St Queens Boro. New York", borough), ());
}

UNIT_TEST(AddressMatcher_RecognizesOnlyHouseAndStreetQueries)
{
  for (auto const * query : {"6498 131a st, surrey", "868 West 67 Ave Vancouver", "122 Main Street"})
    TEST(IsAddressQuery(query), (query));
  for (auto const * query :
       {"coffee", "123 coffee", "131a st Surrey", "6498", "6498 street", "999999999999999999999999 Main Street"})
    TEST(!IsAddressQuery(query), (query));
  TEST(!IsAddressQuery(std::string(10000, '9') + " Main Street"), ());
}

UNIT_TEST(AddressMatcher_DoesNotInventMissingHouseNumbers)
{
  Results source;
  source.AddResultNoChecks(MakeAddress(49.1208601, -122.8575937, "6486, 131A Street"));
  source.AddResultNoChecks(MakeAddress(49.1209318, -122.8577086, "6492, 131A Street"));
  for (auto const * query : {"6490 131a st", "6498 131a st", "6498 131a st Surrey Canada"})
  {
    auto const results = RankAddressResults(query, source);
    TEST_EQUAL(results.GetCount(), source.GetCount(), (query));
    TEST(!FindUniqueAddressResult(query, results), (query));
    for (auto const & result : results)
      TEST(std::any_of(source.begin(), source.end(),
                       [&](Result const & original)
      {
        return original.GetFeatureCenter() == result.GetFeatureCenter() && original.GetString() == result.GetString();
      }),
           ());
  }
}

UNIT_TEST(AddressMatcher_PreservesStreetFirstNearbyResultsAndCancellation)
{
  Results source;
  source.AddResultNoChecks(MakeAddress(0, 0, "131A Street, 6486"));
  source.AddResultNoChecks(MakeAddress(0, 1, "131A Street, 6492"));
  source.SetEndMarker(true);
  auto const results = RankAddressResults("6498 131a st", source);
  TEST_EQUAL(results.GetCount(), 2, ());
  TEST_EQUAL(results[0].GetString(), "131A Street, 6492", ());
  TEST(results.IsEndedCancelled(), ());
  TEST(!FindUniqueAddressResult("6498 131a st", results), ());
}
}  // namespace address_matcher_tests
