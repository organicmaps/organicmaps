#include "testing/testing.hpp"

#include "search/retrieval.hpp"
#include "search/search_integration_tests/helpers.hpp"
#include "search/search_tests_support/test_results_matching.hpp"
#include "search/search_tests_support/test_search_request.hpp"
#include "search/v2/token_slice.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "indexer/feature.hpp"
#include "indexer/index.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/math.hpp"

#include "std/shared_ptr.hpp"
#include "std/vector.hpp"

using namespace generator::tests_support;
using namespace search::tests_support;
using namespace search::v2;

using TRules = vector<shared_ptr<MatchingRule>>;

namespace search
{
namespace
{
void MakeDefaultTestParams(string const & query, SearchParams & params)
{
  params.m_query = query;
  params.m_inputLocale = "en";
  params.SetMode(Mode::Everywhere);
  params.SetSuggestsEnabled(false);
}

class SearchQueryV2Test : public SearchTest
{
};

UNIT_CLASS_TEST(SearchQueryV2Test, Smoke)
{
  string const countryName = "Wonderland";
  TestCountry wonderlandCountry(m2::PointD(10, 10), countryName, "en");

  TestCity losAlamosCity(m2::PointD(10, 10), "Los Alamos", "en", 100 /* rank */);
  TestCity mskCity(m2::PointD(0, 0), "Moscow", "en", 100 /* rank */);
  TestVillage longPondVillage(m2::PointD(15, 15), "Long Pond Village", "en", 10 /* rank */);
  TestStreet feynmanStreet(
      vector<m2::PointD>{m2::PointD(9.999, 9.999), m2::PointD(10, 10), m2::PointD(10.001, 10.001)},
      "Feynman street", "en");
  TestStreet bohrStreet1(
      vector<m2::PointD>{m2::PointD(9.999, 10.001), m2::PointD(10, 10), m2::PointD(10.001, 9.999)},
      "Bohr street", "en");
  TestStreet bohrStreet2(vector<m2::PointD>{m2::PointD(10.001, 9.999), m2::PointD(10.002, 9.998)},
                         "Bohr street", "en");
  TestStreet bohrStreet3(vector<m2::PointD>{m2::PointD(10.002, 9.998), m2::PointD(10.003, 9.997)},
                         "Bohr street", "en");
  TestStreet firstAprilStreet(vector<m2::PointD>{m2::PointD(14.998, 15), m2::PointD(15.002, 15)},
                              "1st April street", "en");

  TestBuilding feynmanHouse(m2::PointD(10, 10), "Feynman house", "1 unit 1", feynmanStreet, "en");
  TestBuilding bohrHouse(m2::PointD(10, 10), "Bohr house", "1 unit 1", bohrStreet1, "en");
  TestBuilding hilbertHouse(
      vector<m2::PointD>{
          {10.0005, 10.0005}, {10.0006, 10.0005}, {10.0006, 10.0006}, {10.0005, 10.0006}},
      "Hilbert house", "1 unit 2", bohrStreet1, "en");
  TestBuilding descartesHouse(m2::PointD(10, 10), "Descartes house", "2", "en");
  TestBuilding bornHouse(m2::PointD(14.999, 15), "Born house", "8", firstAprilStreet, "en");

  TestPOI busStop(m2::PointD(0, 0), "Bus stop", "en");
  TestPOI tramStop(m2::PointD(0.0001, 0.0001), "Tram stop", "en");
  TestPOI quantumTeleport1(m2::PointD(0.0002, 0.0002), "Quantum teleport 1", "en");

  TestPOI quantumTeleport2(m2::PointD(10, 10), "Quantum teleport 2", "en");
  quantumTeleport2.SetHouseNumber("3");
  quantumTeleport2.SetStreet(feynmanStreet);

  TestPOI quantumCafe(m2::PointD(-0.0002, -0.0002), "Quantum cafe", "en");
  TestPOI lantern1(m2::PointD(10.0005, 10.0005), "lantern 1", "en");
  TestPOI lantern2(m2::PointD(10.0006, 10.0005), "lantern 2", "en");

  BuildWorld([&](TestMwmBuilder & builder)
             {
               builder.Add(wonderlandCountry);
               builder.Add(losAlamosCity);
               builder.Add(mskCity);
             });
  auto wonderlandId = BuildCountry(countryName, [&](TestMwmBuilder & builder)
                                   {
                                     builder.Add(losAlamosCity);
                                     builder.Add(mskCity);
                                     builder.Add(longPondVillage);

                                     builder.Add(feynmanStreet);
                                     builder.Add(bohrStreet1);
                                     builder.Add(bohrStreet2);
                                     builder.Add(bohrStreet3);
                                     builder.Add(firstAprilStreet);

                                     builder.Add(feynmanHouse);
                                     builder.Add(bohrHouse);
                                     builder.Add(hilbertHouse);
                                     builder.Add(descartesHouse);
                                     builder.Add(bornHouse);

                                     builder.Add(busStop);
                                     builder.Add(tramStop);
                                     builder.Add(quantumTeleport1);
                                     builder.Add(quantumTeleport2);
                                     builder.Add(quantumCafe);
                                     builder.Add(lantern1);
                                     builder.Add(lantern2);
                                   });

  SetViewport(m2::RectD(m2::PointD(-1.0, -1.0), m2::PointD(1.0, 1.0)));
  {
    TRules rules = {ExactMatch(wonderlandId, busStop)};
    TEST(ResultsMatch("Bus stop", rules), ());
  }
  {
    TRules rules = {ExactMatch(wonderlandId, quantumCafe),
                    ExactMatch(wonderlandId, quantumTeleport1),
                    ExactMatch(wonderlandId, quantumTeleport2)};
    TEST(ResultsMatch("quantum", rules), ());
  }
  {
    TRules rules = {ExactMatch(wonderlandId, quantumCafe),
                    ExactMatch(wonderlandId, quantumTeleport1)};
    TEST(ResultsMatch("quantum Moscow ", rules), ());
  }
  {
    TEST(ResultsMatch("     ", TRules()), ());
  }
  {
    TRules rules = {ExactMatch(wonderlandId, quantumTeleport2)};
    TEST(ResultsMatch("teleport feynman street", rules), ());
  }
  {
    TRules rules = {ExactMatch(wonderlandId, quantumTeleport2)};
    TEST(ResultsMatch("feynman street 3", rules), ());
  }
  {
    TRules rules = {ExactMatch(wonderlandId, feynmanHouse), ExactMatch(wonderlandId, lantern1)};
    TEST(ResultsMatch("feynman street 1", rules), ());
  }
  {
    TRules rules = {ExactMatch(wonderlandId, bohrHouse), ExactMatch(wonderlandId, hilbertHouse),
                    ExactMatch(wonderlandId, lantern1)};
    TEST(ResultsMatch("bohr street 1", rules), ());
  }
  {
    TEST(ResultsMatch("bohr street 1 unit 3", TRules()), ());
  }
  {
    TRules rules = {ExactMatch(wonderlandId, lantern1), ExactMatch(wonderlandId, lantern2)};
    TEST(ResultsMatch("bohr street 1 lantern ", rules), ());
  }
  {
    TRules rules = {ExactMatch(wonderlandId, feynmanHouse)};
    TEST(ResultsMatch("wonderland los alamos feynman 1 unit 1 ", rules), ());
  }
  {
    // It's possible to find Descartes house by name.
    TRules rules = {ExactMatch(wonderlandId, descartesHouse)};
    TEST(ResultsMatch("Los Alamos Descartes", rules), ());
  }
  {
    // It's not possible to find Descartes house by house number,
    // because it doesn't belong to Los Alamos streets. But it still
    // exists.
    TRules rules = {ExactMatch(wonderlandId, lantern2), ExactMatch(wonderlandId, quantumTeleport2)};
    TEST(ResultsMatch("Los Alamos 2", rules), ());
  }
  {
    TRules rules = {ExactMatch(wonderlandId, bornHouse)};
    TEST(ResultsMatch("long pond 1st april street 8", rules), ());
  }
}

UNIT_CLASS_TEST(SearchQueryV2Test, SearchInWorld)
{
  string const countryName = "Wonderland";
  TestCountry wonderland(m2::PointD(0, 0), countryName, "en");
  TestCity losAlamos(m2::PointD(0, 0), "Los Alamos", "en", 100 /* rank */);

  auto testWorldId = BuildWorld([&](TestMwmBuilder & builder)
                                {
                                  builder.Add(wonderland);
                                  builder.Add(losAlamos);
                                });
  RegisterCountry(countryName, m2::RectD(m2::PointD(-1.0, -1.0), m2::PointD(1.0, 1.0)));

  SetViewport(m2::RectD(m2::PointD(-1.0, -1.0), m2::PointD(-0.5, -0.5)));
  {
    TRules rules = {ExactMatch(testWorldId, losAlamos)};
    TEST(ResultsMatch("Los Alamos", rules), ());
  }
  {
    TRules rules = {ExactMatch(testWorldId, wonderland)};
    TEST(ResultsMatch("Wonderland", rules), ());
  }
  {
    TRules rules = {ExactMatch(testWorldId, losAlamos)};
    TEST(ResultsMatch("Wonderland Los Alamos", rules), ());
  }
}

UNIT_CLASS_TEST(SearchQueryV2Test, SearchByName)
{
  string const countryName = "Wonderland";
  TestCity london(m2::PointD(1, 1), "London", "en", 100 /* rank */);
  TestPark hydePark(vector<m2::PointD>{m2::PointD(0.5, 0.5), m2::PointD(1.5, 0.5),
                                       m2::PointD(1.5, 1.5), m2::PointD(0.5, 1.5)},
                    "Hyde Park", "en");
  TestPOI cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");

  auto worldId = BuildWorld([&](TestMwmBuilder & builder)
                            {
                              builder.Add(london);
                            });
  auto wonderlandId = BuildCountry(countryName, [&](TestMwmBuilder & builder)
                                   {
                                     builder.Add(hydePark);
                                     builder.Add(cafe);
                                   });

  SetViewport(m2::RectD(m2::PointD(-1.0, -1.0), m2::PointD(-0.9, -0.9)));
  {
    TRules rules = {ExactMatch(wonderlandId, hydePark)};
    TEST(ResultsMatch("hyde park", rules), ());
    TEST(ResultsMatch("london hyde park", rules), ());
    TEST(ResultsMatch("hyde london park", TRules()), ());
  }

  SetViewport(m2::RectD(m2::PointD(0.5, 0.5), m2::PointD(1.5, 1.5)));
  {
    TRules rules = {ExactMatch(worldId, london)};
    TEST(ResultsMatch("london", Mode::World, rules), ());
  }
  {
    TRules rules = {ExactMatch(worldId, london), ExactMatch(wonderlandId, cafe)};
    TEST(ResultsMatch("london", Mode::Everywhere, rules), ());
  }
}

UNIT_CLASS_TEST(SearchQueryV2Test, DisableSuggests)
{
  TestCity london1(m2::PointD(1, 1), "London", "en", 100 /* rank */);
  TestCity london2(m2::PointD(-1, -1), "London", "en", 100 /* rank */);

  auto worldId = BuildWorld([&](TestMwmBuilder & builder)
                            {
                              builder.Add(london1);
                              builder.Add(london2);
                            });

  SetViewport(m2::RectD(m2::PointD(0.5, 0.5), m2::PointD(1.5, 1.5)));
  {
    SearchParams params;
    params.m_query = "londo";
    params.m_inputLocale = "en";
    params.SetMode(Mode::World);
    params.SetSuggestsEnabled(false);

    TestSearchRequest request(m_engine, params, m_viewport);
    request.Wait();
    TRules rules = {ExactMatch(worldId, london1), ExactMatch(worldId, london2)};

    TEST(MatchResults(m_engine, rules, request.Results()), ());
  }
}

UNIT_CLASS_TEST(SearchQueryV2Test, TestRankingInfo)
{
  string const countryName = "Wonderland";

  TestCity sanFrancisco(m2::PointD(1, 1), "San Francisco", "en", 100 /* rank */);
  // Golden Gate Bridge-bridge is located in this test on the Golden
  // Gate Bridge-street. Therefore, there are several valid parses of
  // the query "Golden Gate Bridge", and search engine must return
  // both features (street and bridge) as they were matched by full
  // name.
  TestStreet goldenGateStreet(
      vector<m2::PointD>{m2::PointD(-0.5, -0.5), m2::PointD(0, 0), m2::PointD(0.5, 0.5)},
      "Golden Gate Bridge", "en");

  TestPOI goldenGateBridge(m2::PointD(0, 0), "Golden Gate Bridge", "en");

  TestPOI waterfall(m2::PointD(0.5, 0.5), "", "en");
  waterfall.SetTypes({{"waterway", "waterfall"}});

  TestPOI lermontov(m2::PointD(1, 1), "Лермонтовъ", "en");
  lermontov.SetTypes({{"amenity", "cafe"}});

  // A city with two noname cafes.
  TestCity lermontovo(m2::PointD(-1, -1), "Лермонтово", "en", 100 /* rank */);
  TestPOI cafe1(m2::PointD(-1.01, -1.01), "", "en");
  cafe1.SetTypes({{"amenity", "cafe"}});
  TestPOI cafe2(m2::PointD(-0.99, -0.99), "", "en");
  cafe2.SetTypes({{"amenity", "cafe"}});

  auto worldId = BuildWorld([&](TestMwmBuilder & builder)
                            {
                              builder.Add(sanFrancisco);
                              builder.Add(lermontovo);
                            });
  auto wonderlandId = BuildCountry(countryName, [&](TestMwmBuilder & builder)
                                   {
                                     builder.Add(cafe1);
                                     builder.Add(cafe2);
                                     builder.Add(goldenGateBridge);
                                     builder.Add(goldenGateStreet);
                                     builder.Add(lermontov);
                                     builder.Add(waterfall);
                                   });

  SetViewport(m2::RectD(m2::PointD(-0.5, -0.5), m2::PointD(0.5, 0.5)));
  {
    SearchParams params;
    MakeDefaultTestParams("golden gate bridge ", params);

    TestSearchRequest request(m_engine, params, m_viewport);
    request.Wait();

    TRules rules = {ExactMatch(wonderlandId, goldenGateBridge),
                    ExactMatch(wonderlandId, goldenGateStreet)};

    TEST(MatchResults(m_engine, rules, request.Results()), ());
    for (auto const & result : request.Results())
    {
      auto const & info = result.GetRankingInfo();
      TEST_EQUAL(NAME_SCORE_FULL_MATCH, info.m_nameScore, (result));
      TEST(my::AlmostEqualAbs(1.0, info.m_nameCoverage, 1e-6), (info.m_nameCoverage));
    }
  }

  // This test is quite important and must always pass.
  {
    SearchParams params;
    MakeDefaultTestParams("cafe лермонтов", params);

    TestSearchRequest request(m_engine, params, m_viewport);
    request.Wait();

    auto const & results = request.Results();

    TRules rules{ExactMatch(wonderlandId, cafe1), ExactMatch(wonderlandId, cafe2),
                 ExactMatch(wonderlandId, lermontov)};
    TEST(MatchResults(m_engine, rules, results), ());

    TEST_EQUAL(3, results.size(), ("Unexpected number of retrieved cafes."));
    auto const & top = results.front();
    TEST(MatchResults(m_engine, {ExactMatch(wonderlandId, lermontov)}, {top}), ());
  }

  {
    TRules rules{ExactMatch(wonderlandId, waterfall)};
    TEST(ResultsMatch("waterfall", rules), ());
  }
}

UNIT_CLASS_TEST(SearchQueryV2Test, TestPostcodes)
{
  string const countryName = "Russia";

  TestCity dolgoprudny(m2::PointD(0, 0), "Долгопрудный", "ru", 100 /* rank */);
  TestCity london(m2::PointD(10, 10), "London", "en", 100 /* rank */);

  TestStreet street(
      vector<m2::PointD>{m2::PointD(-0.5, 0.0), m2::PointD(0, 0), m2::PointD(0.5, 0.0)},
      "Первомайская", "ru");
  TestBuilding building28(m2::PointD(0.0, 0.00001), "", "28а", street, "ru");
  building28.SetPostcode("141701");

  TestBuilding building29(m2::PointD(0.0, -0.00001), "", "29", street, "ru");
  building29.SetPostcode("141701");

  TestBuilding building30(m2::PointD(0.00001, 0.00001), "", "30", street, "ru");
  building30.SetPostcode("141702");

  TestBuilding building1(m2::PointD(10, 10), "", "1", "en");
  building1.SetPostcode("WC2H 7BX");

  BuildWorld([&](TestMwmBuilder & builder)
             {
               builder.Add(dolgoprudny);
               builder.Add(london);
             });
  auto countryId = BuildCountry(countryName, [&](TestMwmBuilder & builder)
                                {
                                  builder.Add(street);
                                  builder.Add(building28);
                                  builder.Add(building29);
                                  builder.Add(building30);

                                  builder.Add(building1);
                                });

  // Tests that postcode is added to the search index.
  {
    auto handle = m_engine.GetMwmHandleById(countryId);
    TEST(handle.IsAlive(), ());
    my::Cancellable cancellable;

    SearchQueryParams params;
    params.m_tokens.emplace_back();
    params.m_tokens.back().push_back(strings::MakeUniString("141702"));
    auto * value = handle.GetValue<MwmValue>();
    auto features = v2::RetrievePostcodeFeatures(countryId, *value, cancellable,
                                                 TokenSlice(params, 0, params.m_tokens.size()));
    TEST_EQUAL(1, features->PopCount(), ());

    uint64_t index = 0;
    while (!features->GetBit(index))
      ++index;

    Index::FeaturesLoaderGuard loader(m_engine, countryId);
    FeatureType ft;
    loader.GetFeatureByIndex(index, ft);

    auto rule = ExactMatch(countryId, building30);
    TEST(rule->Matches(ft), ());
  }

  {
    TRules rules{ExactMatch(countryId, building28)};
    TEST(ResultsMatch("Долгопрудный первомайская 28а", "ru", rules), ());
  }
  {
    TRules rules{ExactMatch(countryId, building28)};
    TEST(ResultsMatch("Долгопрудный первомайская 28а, 141701", "ru", rules), ());
  }
  {
    TRules rules{ExactMatch(countryId, building28), ExactMatch(countryId, building29)};
    TEST(ResultsMatch("Долгопрудный первомайская 141701", "ru", rules), ());

  }
  {
    TRules rules{ExactMatch(countryId, building28), ExactMatch(countryId, building29)};
    TEST(ResultsMatch("Долгопрудный 141701", "ru", rules), ());
  }
  {
    TRules rules{ExactMatch(countryId, building30)};
    TEST(ResultsMatch("Долгопрудный 141702", "ru", rules), ());
  }

  {
    string const kQueries[] = {"london WC2H 7BX", "london WC2H 7", "london WC2H ", "london WC"};
    for (auto const & query : kQueries)
    {
      TRules rules{ExactMatch(countryId, building1)};
      TEST(ResultsMatch(query, rules), (query));
    }
  }
}

UNIT_CLASS_TEST(SearchQueryV2Test, TestCategories)
{
  string const countryName = "Wonderland";

  TestCity sanFrancisco(m2::PointD(0, 0), "San Francisco", "en", 100 /* rank */);

  TestPOI noname(m2::PointD(0, 0), "", "en");
  noname.SetTypes({{"amenity", "atm"}});

  TestPOI named(m2::PointD(0.0001, 0.0001), "ATM", "en");
  named.SetTypes({{"amenity", "atm"}});

  BuildWorld([&](TestMwmBuilder & builder)
             {
               builder.Add(sanFrancisco);
             });
  auto wonderlandId = BuildCountry(countryName, [&](TestMwmBuilder & builder)
                                   {
                                     builder.Add(named);
                                     builder.Add(noname);
                                   });

  SetViewport(m2::RectD(m2::PointD(-0.5, -0.5), m2::PointD(0.5, 0.5)));
  TRules rules = {ExactMatch(wonderlandId, noname), ExactMatch(wonderlandId, named)};

  TEST(ResultsMatch("atm", rules), ());

  {
    SearchParams params;
    MakeDefaultTestParams("#atm", params);

    TestSearchRequest request(m_engine, params, m_viewport);
    request.Wait();

    TEST(MatchResults(m_engine, rules, request.Results()), ());
    for (auto const & result : request.Results())
    {
      auto const & info = result.GetRankingInfo();

      // Token with a hashtag should not participate in name-score
      // calculations.
      TEST_EQUAL(NAME_SCORE_ZERO, info.m_nameScore, (result));

      // TODO (@y): fix this. Name coverage calculations are flawed.
      // TEST(my::AlmostEqualAbs(0.0, info.m_nameCoverage, 1e-6), (info.m_nameCoverage));
    }
  }
}
}  // namespace
}  // namespace search
