#include "testing/testing.hpp"

#include "search/cities_boundaries_table.hpp"
#include "search/features_layer_path_finder.hpp"
#include "search/retrieval.hpp"
#include "search/search_tests_support/helpers.hpp"
#include "search/search_tests_support/test_results_matching.hpp"
#include "search/search_tests_support/test_search_request.hpp"
#include "search/token_range.hpp"
#include "search/token_slice.hpp"

#include "generator/feature_builder.hpp"
#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "editor/editable_data_source.hpp"

#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/macros.hpp"
#include "base/math.hpp"
#include "base/scope_guard.hpp"
#include "base/string_utils.hpp"

#include "std/shared_ptr.hpp"
#include "std/vector.hpp"

using namespace generator::tests_support;
using namespace search::tests_support;

namespace search
{
namespace
{
class TestHotel : public TestPOI
{
public:
  using Type = ftypes::IsHotelChecker::Type;

  TestHotel(m2::PointD const & center, string const & name, string const & lang, float rating,
            int priceRate, Type type)
    : TestPOI(center, name, lang), m_rating(rating), m_priceRate(priceRate)
  {
    CHECK_GREATER_OR_EQUAL(m_rating, 0.0, ());
    CHECK_LESS_OR_EQUAL(m_rating, 10.0, ());

    CHECK_GREATER_OR_EQUAL(m_priceRate, 0, ());
    CHECK_LESS_OR_EQUAL(m_priceRate, 5, ());

    SetTypes({{"tourism", ftypes::IsHotelChecker::GetHotelTypeTag(type)}});
  }

  // TestPOI overrides:
  void Serialize(FeatureBuilder1 & fb) const override
  {
    TestPOI::Serialize(fb);

    auto & metadata = fb.GetMetadata();
    metadata.Set(feature::Metadata::FMD_RATING, strings::to_string(m_rating));
    metadata.Set(feature::Metadata::FMD_PRICE_RATE, strings::to_string(m_priceRate));
  }

private:
  float const m_rating;
  int const m_priceRate;
};

class TestCafeWithCuisine : public TestCafe
{
public:
  TestCafeWithCuisine(m2::PointD const & center, string const & name, string const & lang, string const & cuisine)
    : TestCafe(center, name, lang), m_cuisine(cuisine)
  {
  }

  // TestPOI overrides:
  void Serialize(FeatureBuilder1 & fb) const override
  {
    TestCafe::Serialize(fb);

    auto & metadata = fb.GetMetadata();
    metadata.Set(feature::Metadata::FMD_CUISINE, m_cuisine);
  }

private:
  string m_cuisine;
};

class TestAirport : public TestPOI
{
public:
  TestAirport(m2::PointD const & center, string const & name, string const & lang, string const & iata)
    : TestPOI(center, name, lang), m_iata(iata)
  {
    SetTypes({{"aeroway", "aerodrome"}});
  }

  // TestPOI overrides:
  void Serialize(FeatureBuilder1 & fb) const override
  {
    TestPOI::Serialize(fb);

    auto & metadata = fb.GetMetadata();
    metadata.Set(feature::Metadata::FMD_AIRPORT_IATA, m_iata);
  }

private:
  string m_iata;
};

class TestATM : public TestPOI
{
public:
  TestATM(m2::PointD const & center, string const & op, string const & lang)
    : TestPOI(center, {} /* name */, lang), m_operator(op)
  {
    SetTypes({{"amenity", "atm"}});
  }

  // TestPOI overrides:
  void Serialize(FeatureBuilder1 & fb) const override
  {
    TestPOI::Serialize(fb);

    auto & metadata = fb.GetMetadata();
    metadata.Set(feature::Metadata::FMD_OPERATOR, m_operator);
  }

private:
  string m_operator;
};

class TestBrandFeature : public TestCafe
{
public:
  TestBrandFeature(m2::PointD const & center, string const & brand, string const & lang)
    : TestCafe(center, {} /* name */, lang), m_brand(brand)
  {
  }

  // TestPOI overrides:
  void Serialize(FeatureBuilder1 & fb) const override
  {
    TestCafe::Serialize(fb);

    auto & metadata = fb.GetMetadata();
    metadata.Set(feature::Metadata::FMD_BRAND, m_brand);
  }

private:
  string m_brand;
};

class ProcessorTest : public SearchTest
{
};

UNIT_CLASS_TEST(ProcessorTest, Smoke)
{
  string const countryName = "Wonderland";
  TestCountry wonderlandCountry(m2::PointD(10, 10), countryName, "en");

  TestCity losAlamosCity(m2::PointD(10, 10), "Los Alamos", "en", 100 /* rank */);
  TestCity mskCity(m2::PointD(0, 0), "Moscow", "en", 100 /* rank */);
  TestCity torontoCity(m2::PointD(-10, -10), "Toronto", "en", 100 /* rank */);

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

  TestStreet stradaDrive(vector<m2::PointD>{m2::PointD(-10.001, -10.001), m2::PointD(-10, -10),
                                            m2::PointD(-9.999, -9.999)},
                         "Strada drive", "en");
  TestBuilding terranceHouse(m2::PointD(-10, -10), "", "155", stradaDrive, "en");

  BuildWorld([&](TestMwmBuilder & builder)
             {
               builder.Add(wonderlandCountry);
               builder.Add(losAlamosCity);
               builder.Add(mskCity);
               builder.Add(torontoCity);
             });
  auto wonderlandId = BuildCountry(countryName, [&](TestMwmBuilder & builder)
                                   {
                                     builder.Add(losAlamosCity);
                                     builder.Add(mskCity);
                                     builder.Add(torontoCity);
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

                                     builder.Add(stradaDrive);
                                     builder.Add(terranceHouse);
                                   });

  SetViewport(m2::RectD(m2::PointD(-1.0, -1.0), m2::PointD(1.0, 1.0)));
  {
    TRules rules = {};
    TEST(ResultsMatch("", rules), ());
  }
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
    TRules rules = {ExactMatch(wonderlandId, feynmanHouse), ExactMatch(wonderlandId, lantern1),
                    ExactMatch(wonderlandId, firstAprilStreet)};
//    TEST(ResultsMatch("feynman street 1", rules), ());
  }
  {
    TRules rules = {ExactMatch(wonderlandId, bohrHouse), ExactMatch(wonderlandId, hilbertHouse),
                    ExactMatch(wonderlandId, lantern1), ExactMatch(wonderlandId, firstAprilStreet)};
//    TEST(ResultsMatch("bohr street 1", rules), ());
  }
  {
//    TEST(ResultsMatch("bohr street 1 unit 3", {ExactMatch(wonderlandId, bohrStreet1)}), ());
  }
  {
    TRules rules = {ExactMatch(wonderlandId, lantern1), ExactMatch(wonderlandId, lantern2)};
    TEST(ResultsMatch("bohr street 1 lantern ", rules), ());
  }
  {
    TRules rules = {ExactMatch(wonderlandId, feynmanHouse),
                    ExactMatch(wonderlandId, feynmanStreet)};
//    TEST(ResultsMatch("wonderland los alamos feynman 1 unit 1 ", rules), ());
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
    TRules rules = {ExactMatch(wonderlandId, bornHouse),
                    ExactMatch(wonderlandId, firstAprilStreet)};
//    TEST(ResultsMatch("long pond 1st april street 8 ", rules), ());
  }

  {
    TRules rules = {ExactMatch(wonderlandId, terranceHouse), ExactMatch(wonderlandId, stradaDrive)};
//    TEST(ResultsMatch("Toronto strada drive 155", rules), ());
  }
}

UNIT_CLASS_TEST(ProcessorTest, SearchInWorld)
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

UNIT_CLASS_TEST(ProcessorTest, SearchByName)
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
    TEST(ResultsMatch("london", Mode::Downloader, rules), ());
  }
  {
    TRules rules = {ExactMatch(worldId, london), ExactMatch(wonderlandId, cafe)};
    TEST(ResultsMatch("london", Mode::Everywhere, rules), ());
  }
}

UNIT_CLASS_TEST(ProcessorTest, DisableSuggests)
{
  TestCity london1(m2::PointD(1, 1), "London", "en", 100 /* rank */);
  TestCity london2(m2::PointD(-1, -1), "London", "en", 100 /* rank */);

  auto worldId = BuildWorld([&](TestMwmBuilder & builder)
                            {
                              builder.Add(london1);
                              builder.Add(london2);
                            });

  {
    SearchParams params;
    params.m_query = "londo";
    params.m_inputLocale = "en";
    params.m_viewport = m2::RectD(m2::PointD(0.5, 0.5), m2::PointD(1.5, 1.5));
    params.m_mode = Mode::Downloader;
    params.m_suggestsEnabled = false;

    TestSearchRequest request(m_engine, params);
    request.Run();
    TRules rules = {ExactMatch(worldId, london1), ExactMatch(worldId, london2)};

    TEST(ResultsMatch(request.Results(), rules), ());
  }
}

UNIT_CLASS_TEST(ProcessorTest, TestRankingInfo)
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

  // A low-rank city with two noname cafes.
  TestCity lermontovo(m2::PointD(-1, -1), "Лермонтово", "en", 0 /* rank */);
  TestPOI cafe1(m2::PointD(-1.01, -1.01), "", "en");
  cafe1.SetTypes({{"amenity", "cafe"}});
  TestPOI cafe2(m2::PointD(-0.99, -0.99), "", "en");
  cafe2.SetTypes({{"amenity", "cafe"}});

  // A low-rank village with a single noname cafe.
  TestVillage pushkino(m2::PointD(-10, -10), "Pushkino", "en", 0 /* rank */);
  TestPOI cafe3(m2::PointD(-10.01, -10.01), "", "en");
  cafe3.SetTypes({{"amenity", "cafe"}});

  auto worldId = BuildWorld([&](TestMwmBuilder & builder)
                            {
                              builder.Add(sanFrancisco);
                              builder.Add(lermontovo);
                            });
  auto wonderlandId = BuildCountry(countryName, [&](TestMwmBuilder & builder)
                                   {
                                     builder.Add(cafe1);
                                     builder.Add(cafe2);
                                     builder.Add(cafe3);
                                     builder.Add(goldenGateBridge);
                                     builder.Add(goldenGateStreet);
                                     builder.Add(lermontov);
                                     builder.Add(pushkino);
                                     builder.Add(waterfall);
                                   });

  SetViewport(m2::RectD(m2::PointD(-0.5, -0.5), m2::PointD(0.5, 0.5)));
  {
    auto request = MakeRequest("golden gate bridge ");

    TRules rules = {ExactMatch(wonderlandId, goldenGateBridge),
                    ExactMatch(wonderlandId, goldenGateStreet)};

    TEST(ResultsMatch(request->Results(), rules), ());
    for (auto const & result : request->Results())
    {
      auto const & info = result.GetRankingInfo();
      TEST_EQUAL(NAME_SCORE_FULL_MATCH, info.m_nameScore, (result));
      TEST(!info.m_pureCats, (result));
      TEST(!info.m_falseCats, (result));
    }
  }

  // This test is quite important and must always pass.
  {
    auto request = MakeRequest("cafe лермонтов");
    auto const & results = request->Results();

    TRules rules{ExactMatch(wonderlandId, cafe1), ExactMatch(wonderlandId, cafe2),
                 ExactMatch(wonderlandId, lermontov)};
    TEST(ResultsMatch(results, rules), ());

    TEST_EQUAL(3, results.size(), ("Unexpected number of retrieved cafes."));
    auto const & top = results.front();
    TEST(ResultsMatch({top}, {ExactMatch(wonderlandId, lermontov)}), ());
  }

  {
    TRules rules{ExactMatch(wonderlandId, waterfall)};
    TEST(ResultsMatch("waterfall", rules), ());
  }

  SetViewport(m2::RectD(m2::PointD(-10.5, -10.5), m2::PointD(-9.5, -9.5)));
  {
    TRules rules{ExactMatch(wonderlandId, cafe3)};
    TEST(ResultsMatch("cafe pushkino ", rules), ());
  }
}

UNIT_CLASS_TEST(ProcessorTest, TestRankingInfo_ErrorsMade)
{
  string const countryName = "Wonderland";

  TestCity chekhov(m2::PointD(0, 0), "Чеховъ Антонъ Павловичъ", "ru", 100 /* rank */);

  TestStreet yesenina(
      vector<m2::PointD>{m2::PointD(0.5, -0.5), m2::PointD(0, 0), m2::PointD(-0.5, 0.5)},
      "Yesenina street", "en");

  TestStreet pushkinskaya(
      vector<m2::PointD>{m2::PointD(-0.5, -0.5), m2::PointD(0, 0), m2::PointD(0.5, 0.5)},
      "Улица Пушкинская", "ru");

  TestStreet ostrovskogo(
      vector<m2::PointD>{m2::PointD(-0.5, 0.0), m2::PointD(0, 0), m2::PointD(0.5, 0.0)},
      "улица Островского", "ru");

  TestPOI lermontov(m2::PointD(0, 0), "Трактиръ Лермонтовъ", "ru");
  lermontov.SetTypes({{"amenity", "cafe"}});

  auto worldId = BuildWorld([&](TestMwmBuilder & builder) { builder.Add(chekhov); });

  auto wonderlandId = BuildCountry(countryName, [&](TestMwmBuilder & builder) {
    builder.Add(yesenina);
    builder.Add(pushkinskaya);
    builder.Add(ostrovskogo);
    builder.Add(lermontov);
  });

  SetViewport(m2::RectD(-1, -1, 1, 1));

  auto checkErrors = [&](string const & query, ErrorsMade const & errorsMade) {
    auto request = MakeRequest(query, "ru");
    auto const & results = request->Results();

    TRules rules{ExactMatch(wonderlandId, lermontov)};
    TEST(ResultsMatch(results, rules), (query));
    TEST_EQUAL(results.size(), 1, (query));
    TEST_EQUAL(results[0].GetRankingInfo().m_errorsMade, errorsMade, (query));
  };

  checkErrors("кафе лермонтов", ErrorsMade(1));
  checkErrors("трактир лермонтов", ErrorsMade(2));
  checkErrors("кафе", ErrorsMade());

  checkErrors("Yesenina cafe", ErrorsMade(0));
  checkErrors("Esenina cafe", ErrorsMade(1));
  checkErrors("Jesenina cafe", ErrorsMade(1));

  checkErrors("Островского кафе", ErrorsMade(0));
  checkErrors("Астровского кафе", ErrorsMade(1));

  checkErrors("пушкенская трактир лермонтов", ErrorsMade(3));
  checkErrors("пушкенская кафе", ErrorsMade(1));
  checkErrors("пушкинская трактиръ лермонтовъ", ErrorsMade(0));

  checkErrors("лермонтовъ чехов", ErrorsMade(1));
  checkErrors("лермонтовъ чеховъ", ErrorsMade(0));
  checkErrors("лермонтов чехов", ErrorsMade(2));
  checkErrors("лермонтов чеховъ", ErrorsMade(1));

  checkErrors("лермонтов чеховъ антон павлович", ErrorsMade(3));
}

UNIT_CLASS_TEST(ProcessorTest, TestHouseNumbers)
{
  string const countryName = "HouseNumberLand";

  TestCity greenCity(m2::PointD(0, 0), "Зеленоград", "ru", 100 /* rank */);

  TestStreet street(
      vector<m2::PointD>{m2::PointD(0.0, -10.0), m2::PointD(0, 0), m2::PointD(5.0, 5.0)},
      "Генерала Генералова", "ru");

  TestBuilding building0(m2::PointD(2.0, 2.0), "", "100", "en");
  TestBuilding building1(m2::PointD(3.0, 3.0), "", "к200", "ru");
  TestBuilding building2(m2::PointD(4.0, 4.0), "", "300 строение 400", "ru");

  BuildWorld([&](TestMwmBuilder & builder) { builder.Add(greenCity); });
  auto countryId = BuildCountry(countryName, [&](TestMwmBuilder & builder) {
    builder.Add(street);
    builder.Add(building0);
    builder.Add(building1);
    builder.Add(building2);
  });

  {
    TRules rules{ExactMatch(countryId, building0), ExactMatch(countryId, street)};
//    TEST(ResultsMatch("Зеленоград генералова к100 ", "ru", rules), ());
  }
  {
    TRules rules{ExactMatch(countryId, building1), ExactMatch(countryId, street)};
//    TEST(ResultsMatch("Зеленоград генералова к200 ", "ru", rules), ());
  }
  {
    TRules rules{ExactMatch(countryId, building1), ExactMatch(countryId, street)};
//    TEST(ResultsMatch("Зеленоград к200 генералова ", "ru", rules), ());
  }
  {
    TRules rules{ExactMatch(countryId, building2), ExactMatch(countryId, street)};
//    TEST(ResultsMatch("Зеленоград 300 строение 400 генералова ", "ru", rules), ());
  }
  {
    TRules rules{ExactMatch(countryId, street)};
//    TEST(ResultsMatch("Зеленоград генералова строе 300", "ru", rules), ());
  }
  {
    TRules rules{ExactMatch(countryId, building2), ExactMatch(countryId, street)};
//    TEST(ResultsMatch("Зеленоград генералова 300 строе", "ru", rules), ());
  }
}

UNIT_CLASS_TEST(ProcessorTest, TestPostcodes)
{
  string const countryName = "Russia";

  TestCity dolgoprudny(m2::PointD(0, 0), "Долгопрудный", "ru", 100 /* rank */);
  TestCity london(m2::PointD(10, 10), "London", "en", 100 /* rank */);

  TestStreet street(
      vector<m2::PointD>{m2::PointD(-0.5, 0.0), m2::PointD(0, 0), m2::PointD(0.5, 0.0)},
      "Первомайская", "ru");
  street.SetPostcode("141701");

  TestBuilding building28(m2::PointD(0.0, 0.00001), "", "28а", street, "ru");
  building28.SetPostcode("141701");

  TestBuilding building29(m2::PointD(0.0, -0.00001), "", "29", street, "ru");
  building29.SetPostcode("141701");

  TestPOI building30(m2::PointD(0.00002, 0.00002), "", "en");
  building30.SetHouseNumber("30");
  building30.SetPostcode("141701");
  building30.SetTypes({{"building", "address"}});

  TestBuilding building31(m2::PointD(0.00001, 0.00001), "", "31", street, "ru");
  building31.SetPostcode("141702");

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
                                  builder.Add(building31);

                                  builder.Add(building1);
                                });

  // Tests that postcode is added to the search index.
  {
    MwmContext context(m_dataSource.GetMwmHandleById(countryId));
    base::Cancellable cancellable;

    QueryParams params;
    {
      strings::UniString const tokens[] = {strings::MakeUniString("141702")};
      params.InitNoPrefix(tokens, tokens + ARRAY_SIZE(tokens));
    }

    Retrieval retrieval(context, cancellable);
    auto features = retrieval.RetrievePostcodeFeatures(
        TokenSlice(params, TokenRange(0, params.GetNumTokens())));
    TEST_EQUAL(1, features->PopCount(), ());

    uint64_t index = 0;
    while (!features->GetBit(index))
      ++index;

    FeaturesLoaderGuard loader(m_dataSource, countryId);
    FeatureType ft;
    TEST(loader.GetFeatureByIndex(base::checked_cast<uint32_t>(index), ft), ());

    auto rule = ExactMatch(countryId, building31);
    TEST(rule->Matches(ft), ());
  }

  {
    TRules rules{ExactMatch(countryId, building28), ExactMatch(countryId, street)};
//    TEST(ResultsMatch("Долгопрудный первомайская 28а ", "ru", rules), ());
  }
  {
    TRules rules{ExactMatch(countryId, building28), ExactMatch(countryId, street)};
//    TEST(ResultsMatch("Долгопрудный первомайская 28а, 141701", "ru", rules), ());
  }
  {
    TRules rules{ExactMatch(countryId, building28), ExactMatch(countryId, building29),
                 ExactMatch(countryId, building30), ExactMatch(countryId, street)};
    TEST(ResultsMatch("Долгопрудный первомайская 141701", "ru", rules), ());
  }
  {
    TRules rules{ExactMatch(countryId, building31), ExactMatch(countryId, street)};
//    TEST(ResultsMatch("Долгопрудный первомайская 141702", "ru", rules), ());
  }
  {
    TRules rules{ExactMatch(countryId, building28), ExactMatch(countryId, building29),
                 ExactMatch(countryId, building30), ExactMatch(countryId, street)};
    TEST(ResultsMatch("Долгопрудный 141701", "ru", rules), ());
  }
  {
    TRules rules{ExactMatch(countryId, building31)};
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

UNIT_CLASS_TEST(ProcessorTest, TestCategories)
{
  string const countryName = "Wonderland";

  TestCity sanFrancisco(m2::PointD(0, 0), "San Francisco", "en", 100 /* rank */);

  TestPOI nonameBeach(m2::PointD(0, 0), "", "ru");
  nonameBeach.SetTypes({{"natural", "beach"}});

  TestPOI namedBeach(m2::PointD(0.2, 0.2), "San Francisco beach", "en");
  namedBeach.SetTypes({{"natural", "beach"}});

  TestPOI nonameAtm(m2::PointD(0, 0), "", "en");
  nonameAtm.SetTypes({{"amenity", "atm"}});

  TestPOI namedAtm(m2::PointD(0.3, 0.3), "ATM", "en");
  namedAtm.SetTypes({{"amenity", "atm"}});

  TestPOI busStop(m2::PointD(0.00005, 0.0005), "ATM Bus Stop", "en");
  busStop.SetTypes({{"highway", "bus_stop"}});

  TestPOI cafe(m2::PointD(0.0001, 0.0001), "Cafe", "en");
  cafe.SetTypes({{"amenity", "cafe"}, {"internet_access", "wlan"}});

  TestPOI toi(m2::PointD(0.0001, 0.0001), "", "en");
  toi.SetTypes({{"amenity", "toilets"}});

  BuildWorld([&](TestMwmBuilder & builder)
             {
               builder.Add(sanFrancisco);
             });
  auto wonderlandId = BuildCountry(countryName, [&](TestMwmBuilder & builder)
                                   {
                                     builder.Add(busStop);
                                     builder.Add(cafe);
                                     builder.Add(namedAtm);
                                     builder.Add(namedBeach);
                                     builder.Add(nonameAtm);
                                     builder.Add(nonameBeach);
                                     builder.Add(toi);
                                   });

  SetViewport(m2::RectD(m2::PointD(-0.5, -0.5), m2::PointD(0.5, 0.5)));

  {
    TRules const rules = {ExactMatch(wonderlandId, nonameAtm), ExactMatch(wonderlandId, namedAtm),
                          ExactMatch(wonderlandId, busStop)};

    auto request = MakeRequest("atm");
    TEST(ResultsMatch(request->Results(), rules), ());
    for (auto const & result : request->Results())
    {
      FeaturesLoaderGuard loader(m_dataSource, wonderlandId);
      FeatureType ft;
      TEST(loader.GetFeatureByIndex(result.GetFeatureID().m_index, ft), ());

      auto const & info = result.GetRankingInfo();

      if (busStop.Matches(ft))
      {
        TEST(!info.m_pureCats, (result));
        TEST(info.m_falseCats, (result));
      }
      else
      {
        TEST(info.m_pureCats, (result));
        TEST(!info.m_falseCats, (result));
      }
    }
  }

  TEST(ResultsMatch("wifi", {ExactMatch(wonderlandId, cafe)}), ());
  TEST(ResultsMatch("wi-fi", {ExactMatch(wonderlandId, cafe)}), ());
  TEST(ResultsMatch("wai-fai", TRules{}), ());
  TEST(ResultsMatch("toilet", {ExactMatch(wonderlandId, toi)}), ());
  TEST(ResultsMatch("beach ",
                    {ExactMatch(wonderlandId, nonameBeach), ExactMatch(wonderlandId, namedBeach)}),
       ());
}

// A separate test for the categorial search branch in the geocoder.
UNIT_CLASS_TEST(ProcessorTest, TestCategorialSearch)
{
  string const countryName = "Wonderland";

  TestCity sanDiego(m2::PointD(0, 0), "San Diego", "en", 100 /* rank */);
  TestCity homel(m2::PointD(10, 10), "Homel", "en", 100 /* rank */);

  // No need in TestHotel here, TestPOI is enough.
  TestPOI hotel1(m2::PointD(0, 0.01), "", "ru");
  hotel1.SetTypes({{"tourism", "hotel"}});

  TestPOI hotel2(m2::PointD(0, 0.02), "Hotel San Diego, California", "en");
  hotel2.SetTypes({{"tourism", "hotel"}});

  TestPOI hotelCafe(m2::PointD(0, 0.03), "Hotel", "en");
  hotelCafe.SetTypes({{"amenity", "cafe"}});

  TestPOI hotelDeVille(m2::PointD(0, 0.04), "Hôtel De Ville", "en");
  hotelDeVille.SetTypes({{"amenity", "townhall"}});

  TestPOI nightclub(m2::PointD(0, 0.05), "Moulin Rouge", "fr");
  nightclub.SetTypes({{"amenity", "nightclub"}});

  // A POI with that matches "entertainment" only by name.
  TestPOI laundry(m2::PointD(0, 0.06), "Entertainment 720", "en");
  laundry.SetTypes({{"shop", "laundry"}});

  auto const testWorldId = BuildWorld([&](TestMwmBuilder & builder) {
    builder.Add(sanDiego);
    builder.Add(homel);
  });
  auto wonderlandId = BuildCountry(countryName, [&](TestMwmBuilder & builder) {
    builder.Add(hotel1);
    builder.Add(hotel2);
    builder.Add(hotelCafe);
    builder.Add(hotelDeVille);
    builder.Add(nightclub);
    builder.Add(laundry);
  });

  SetViewport(m2::RectD(m2::PointD(-0.5, -0.5), m2::PointD(0.5, 0.5)));

  {
    TRules const rules = {ExactMatch(wonderlandId, hotel1), ExactMatch(wonderlandId, hotel2)};

    TEST(ResultsMatch("hotel ", rules), ());
    TEST(ResultsMatch("hôTeL ", rules), ());
  }

  {
    TRules const rules = {ExactMatch(wonderlandId, nightclub)};

    // A category with a rare name. The word "Entertainment"
    // occurs exactly once in the list of categories and starts
    // with a capital letter. This is a test for normalization.
    TEST(ResultsMatch("entertainment ", rules), ());
  }

  {
    TRules const rules = {ExactMatch(wonderlandId, hotel1), ExactMatch(wonderlandId, hotel2)};

    auto request = MakeRequest("гостиница ", "ru");
    TEST(ResultsMatch(request->Results(), rules), ());
  }

  {
    TRules const rules = {ExactMatch(wonderlandId, hotel1), ExactMatch(wonderlandId, hotel2)};

    // Hotel unicode character: both a synonym and and emoji.
    uint32_t const hotelEmojiCodepoint = 0x0001F3E8;
    strings::UniString const hotelUniString(1, hotelEmojiCodepoint);
    auto request = MakeRequest(ToUtf8(hotelUniString));
    TEST(ResultsMatch(request->Results(), rules), ());
  }

  {
    TRules const rules = {ExactMatch(wonderlandId, hotel1), ExactMatch(wonderlandId, hotel2),
                          ExactMatch(wonderlandId, hotelCafe), ExactMatch(testWorldId, homel),
                          ExactMatch(wonderlandId, hotelDeVille)};
    // A prefix token.
    auto request = MakeRequest("hotel");
    TEST(ResultsMatch(request->Results(), rules), ());
  }

  {
    TRules const rules = {ExactMatch(wonderlandId, hotel1), ExactMatch(wonderlandId, hotel2),
                          ExactMatch(wonderlandId, hotelCafe),
                          ExactMatch(wonderlandId, hotelDeVille)};
    // It looks like a category search but we cannot tell it, so
    // even the features that match only by name are emitted.
    TEST(ResultsMatch("hotel san diego ", rules), ());
  }

  {
    TRules const rules = {ExactMatch(wonderlandId, hotel1), ExactMatch(wonderlandId, hotel2),
                          ExactMatch(wonderlandId, hotelCafe), ExactMatch(testWorldId, homel),
                          ExactMatch(wonderlandId, hotelDeVille)};
    // Homel matches exactly, other features are matched by fuzzy names.
    TEST(ResultsMatch("homel ", rules), ());
  }

  {
    TRules const rules = {ExactMatch(wonderlandId, hotel1), ExactMatch(wonderlandId, hotel2),
                          ExactMatch(wonderlandId, hotelCafe), ExactMatch(testWorldId, homel),
                          ExactMatch(wonderlandId, hotelDeVille)};
    // A typo in search: all features fit.
    TEST(ResultsMatch("hofel ", rules), ());
  }

  {
    TRules const rules = {ExactMatch(wonderlandId, hotelDeVille)};

    TEST(ResultsMatch("hotel de ville ", rules), ());
  }
}

UNIT_CLASS_TEST(ProcessorTest, TestCoords)
{
  auto request = MakeRequest("51.681644 39.183481");
  auto const & results = request->Results();
  TEST_EQUAL(results.size(), 1, ());

  auto const & result = results[0];
  TEST_EQUAL(result.GetResultType(), Result::Type::LatLon, ());
  TEST(result.HasPoint(), ());

  m2::PointD const expected = MercatorBounds::FromLatLon(51.681644, 39.183481);
  auto const actual = result.GetFeatureCenter();
  TEST(MercatorBounds::DistanceOnEarth(expected, actual) <= 1.0, ());
}

UNIT_CLASS_TEST(ProcessorTest, HotelsFiltering)
{
  char const countryName[] = "Wonderland";

  TestHotel h1(m2::PointD(0, 0), "h1", "en", 8.0 /* rating */, 2 /* priceRate */,
               TestHotel::Type::Hotel);
  TestHotel h2(m2::PointD(0, 1), "h2", "en", 7.0 /* rating */, 5 /* priceRate */,
               TestHotel::Type::Hostel);
  TestHotel h3(m2::PointD(1, 0), "h3", "en", 9.0 /* rating */, 0 /* priceRate */,
               TestHotel::Type::GuestHouse);
  TestHotel h4(m2::PointD(1, 1), "h4", "en", 2.0 /* rating */, 4 /* priceRate */,
               TestHotel::Type::GuestHouse);

  auto id = BuildCountry(countryName, [&](TestMwmBuilder & builder) {
    builder.Add(h1);
    builder.Add(h2);
    builder.Add(h3);
    builder.Add(h4);
  });

  SearchParams params;
  params.m_query = "hotel";
  params.m_inputLocale = "en";
  params.m_viewport = m2::RectD(m2::PointD(-1, -1), m2::PointD(2, 2));
  params.m_mode = Mode::Everywhere;
  params.m_suggestsEnabled = false;

  {
    TRules rules = {ExactMatch(id, h1), ExactMatch(id, h2), ExactMatch(id, h3), ExactMatch(id, h4)};
    TEST(ResultsMatch(params, rules), ());
  }

  using namespace hotels_filter;

  params.m_hotelsFilter = And(Gt<Rating>(7.0), Le<PriceRate>(2));
  {
    TRules rules = {ExactMatch(id, h1), ExactMatch(id, h3)};
    TEST(ResultsMatch(params, rules), ());
  }

  params.m_hotelsFilter = Or(Eq<Rating>(9.0), Le<PriceRate>(4));
  {
    TRules rules = {ExactMatch(id, h1), ExactMatch(id, h3), ExactMatch(id, h4)};
    TEST(ResultsMatch(params, rules), ());
  }

  params.m_hotelsFilter = Or(And(Eq<Rating>(7.0), Eq<PriceRate>(5)), Eq<PriceRate>(4));
  {
    TRules rules = {ExactMatch(id, h2), ExactMatch(id, h4)};
    TEST(ResultsMatch(params, rules), ());
  }

  params.m_hotelsFilter = Or(Is(TestHotel::Type::GuestHouse), Is(TestHotel::Type::Hostel));
  {
    TRules rules = {ExactMatch(id, h2), ExactMatch(id, h3), ExactMatch(id, h4)};
    TEST(ResultsMatch(params, rules), ());
  }

  params.m_hotelsFilter = And(Gt<PriceRate>(3), Is(TestHotel::Type::GuestHouse));
  {
    TRules rules = {ExactMatch(id, h4)};
    TEST(ResultsMatch(params, rules), ());
  }

  params.m_hotelsFilter = OneOf((1U << static_cast<unsigned>(TestHotel::Type::Hotel)) |
                                (1U << static_cast<unsigned>(TestHotel::Type::Hostel)));
  {
    TRules rules = {ExactMatch(id, h1), ExactMatch(id, h2)};
    TEST(ResultsMatch(params, rules), ());
  }
}

UNIT_CLASS_TEST(ProcessorTest, FuzzyMatch)
{
  string const countryName = "Wonderland";
  TestCountry country(m2::PointD(10, 10), countryName, "en");

  TestCity city(m2::PointD(0, 0), "Москва", "ru", 100 /* rank */);
  TestStreet street(vector<m2::PointD>{m2::PointD(-0.001, -0.001), m2::PointD(0.001, 0.001)},
                    "Ленинградский", "ru");
  TestPOI bar(m2::PointD(0, 0), "Черчилль", "ru");
  bar.SetTypes({{"amenity", "pub"}});

  TestPOI metro(m2::PointD(5.0, 5.0), "Liceu", "es");
  metro.SetTypes({{"railway", "subway_entrance"}});

  BuildWorld([&](TestMwmBuilder & builder) {
    builder.Add(country);
    builder.Add(city);
  });

  auto id = BuildCountry(countryName, [&](TestMwmBuilder & builder) {
    builder.Add(street);
    builder.Add(bar);
    builder.Add(metro);
  });

  SetViewport(m2::RectD(m2::PointD(-1.0, -1.0), m2::PointD(1.0, 1.0)));
  {
    TRules rulesWithoutStreet = {ExactMatch(id, bar)};
    TRules rules = {ExactMatch(id, bar), ExactMatch(id, street)};
    TEST(ResultsMatch("москва черчилль", "ru", rulesWithoutStreet), ());
//    TEST(ResultsMatch("москва ленинградский черчилль", "ru", rules), ());
//    TEST(ResultsMatch("москва ленинградский паб черчилль", "ru", rules), ());

//    TEST(ResultsMatch("масква лининградский черчиль", "ru", rules), ());
//    TEST(ResultsMatch("масква ленинргадский черчиль", "ru", rules), ());

    // Too many errors, can't do anything.
    TEST(ResultsMatch("масква лениноргадсский чирчиль", "ru", TRules{}), ());

//    TEST(ResultsMatch("моксва ленинргадский черчиль", "ru", rules), ());

    TEST(ResultsMatch("food", "ru", rulesWithoutStreet), ());
    TEST(ResultsMatch("foood", "ru", rulesWithoutStreet), ());
    TEST(ResultsMatch("fod", "ru", TRules{}), ());

    TRules rulesMetro = {ExactMatch(id, metro)};
    TEST(ResultsMatch("transporte", "es", rulesMetro), ());
    TEST(ResultsMatch("transport", "es", rulesMetro), ());
    TEST(ResultsMatch("transpurt", "en", rulesMetro), ());
    TEST(ResultsMatch("transpurrt", "es", rulesMetro), ());
    TEST(ResultsMatch("transportation", "en", TRules{}), ());
  }
}

UNIT_CLASS_TEST(ProcessorTest, SpacesInCategories)
{
  string const countryName = "Wonderland";
  TestCountry country(m2::PointD(10, 10), countryName, "en");

  TestCity city(m2::PointD(5.0, 5.0), "Москва", "ru", 100 /* rank */);
  TestPOI nightclub(m2::PointD(5.0, 5.0), "Crasy daizy", "ru");
  nightclub.SetTypes({{"amenity", "nightclub"}});

  BuildWorld([&](TestMwmBuilder & builder) {
    builder.Add(country);
    builder.Add(city);
  });

  auto id = BuildCountry(countryName, [&](TestMwmBuilder & builder) { builder.Add(nightclub); });

  {
    TRules rules = {ExactMatch(id, nightclub)};
    TEST(ResultsMatch("nightclub", "en", rules), ());
    TEST(ResultsMatch("night club", "en", rules), ());
    TEST(ResultsMatch("n i g h t c l u b", "en", TRules{}), ());
    TEST(ResultsMatch("Москва ночной клуб", "ru", rules), ());
  }
}

UNIT_CLASS_TEST(ProcessorTest, StopWords)
{
  TestCountry country(m2::PointD(0, 0), "France", "en");
  TestCity city(m2::PointD(0, 0), "Paris", "en", 100 /* rank */);
  TestStreet street(
      vector<m2::PointD>{m2::PointD(-0.001, -0.001), m2::PointD(0, 0), m2::PointD(0.001, 0.001)},
      "Rue de la Paix", "en");

  TestPOI bakery(m2::PointD(0.0, 0.0), "" /* name */, "en");
  bakery.SetTypes({{"shop", "bakery"}});

  BuildWorld([&](TestMwmBuilder & builder) {
    builder.Add(country);
    builder.Add(city);
  });

  auto id = BuildCountry(country.GetName(), [&](TestMwmBuilder & builder) {
    builder.Add(street);
    builder.Add(bakery);
  });

  {
    auto request = MakeRequest("la France à Paris Rue de la Paix");

    TRules rules = {ExactMatch(id, street)};

    auto const & results = request->Results();
    TEST(ResultsMatch(results, rules), ());

    auto const & info = results[0].GetRankingInfo();
    TEST_EQUAL(info.m_nameScore, NAME_SCORE_FULL_MATCH, ());
  }

  {
    TRules rules = {ExactMatch(id, bakery)};

    TEST(ResultsMatch("la boulangerie ", "fr", rules), ());
  }

  {
    TEST(ResultsMatch("la motviderie ", "fr", TRules{}), ());
//    TEST(ResultsMatch("la la le la la la ", "fr", {ExactMatch(id, street)}), ());
    TEST(ResultsMatch("la la le la la la", "fr", TRules{}), ());
  }
}

UNIT_CLASS_TEST(ProcessorTest, Numerals)
{
  TestCountry country(m2::PointD(0, 0), "Беларусь", "ru");
  TestPOI school(m2::PointD(0, 0), "СШ №61", "ru");
  school.SetTypes({{"amenity", "school"}});

  auto id = BuildCountry(country.GetName(), [&](TestMwmBuilder & builder) { builder.Add(school); });

  SetViewport(m2::RectD(m2::PointD(-1.0, -1.0), m2::PointD(1.0, 1.0)));
  {
    TRules rules{ExactMatch(id, school)};
    TEST(ResultsMatch("Школа 61", "ru", rules), ());
    TEST(ResultsMatch("Школа # 61", "ru", rules), ());
    TEST(ResultsMatch("school #61", "ru", rules), ());
    TEST(ResultsMatch("сш №61", "ru", rules), ());
    TEST(ResultsMatch("школа", "ru", rules), ());
  }
}

UNIT_CLASS_TEST(ProcessorTest, TestWeirdTypes)
{
  string const countryName = "Area51";

  TestCity tokyo(m2::PointD(0, 0), "東京", "ja", 100 /* rank */);

  TestStreet street(vector<m2::PointD>{m2::PointD(-8.0, 0.0), m2::PointD(8.0, 0.0)}, "竹下通り",
                    "ja");

  TestPOI defibrillator1(m2::PointD(0.0, 0.0), "" /* name */, "en");
  defibrillator1.SetTypes({{"emergency", "defibrillator"}});
  TestPOI defibrillator2(m2::PointD(-5.0, 0.0), "" /* name */, "ja");
  defibrillator2.SetTypes({{"emergency", "defibrillator"}});
  TestPOI fireHydrant(m2::PointD(2.0, 0.0), "" /* name */, "en");
  fireHydrant.SetTypes({{"emergency", "fire_hydrant"}});

  BuildWorld([&](TestMwmBuilder & builder) { builder.Add(tokyo); });

  auto countryId = BuildCountry(countryName, [&](TestMwmBuilder & builder) {
    builder.Add(street);
    builder.Add(defibrillator1);
    builder.Add(defibrillator2);
    builder.Add(fireHydrant);
  });

  {
    TRules rules{ExactMatch(countryId, defibrillator1), ExactMatch(countryId, defibrillator2)};
    TEST(ResultsMatch("defibrillator", "en", rules), ());
    TEST(ResultsMatch("除細動器", "ja", rules), ());

    TRules onlyFirst{ExactMatch(countryId, defibrillator1)};
    TRules firstWithStreet{ExactMatch(countryId, defibrillator1), ExactMatch(countryId, street)};

    // City + category. Only the first defibrillator is inside.
    TEST(ResultsMatch("東京 除細動器 ", "ja", onlyFirst), ());

    // City + street + category.
//    TEST(ResultsMatch("東京 竹下通り 除細動器 ", "ja", firstWithStreet), ());
  }

  {
    TRules rules{ExactMatch(countryId, fireHydrant)};
    TEST(ResultsMatch("fire hydrant", "en", rules), ());
    TEST(ResultsMatch("гидрант", "ru", rules), ());
    TEST(ResultsMatch("пожарный гидрант", "ru", rules), ());

    TEST(ResultsMatch("fire station", "en", TRules{}), ());
  }
}

UNIT_CLASS_TEST(ProcessorTest, CityBoundaryLoad)
{
  TestCity city(vector<m2::PointD>({m2::PointD(0, 0), m2::PointD(0.5, 0), m2::PointD(0.5, 0.5),
                                    m2::PointD(0, 0.5)}),
                "moscow", "en", 100 /* rank */);
  auto const id = BuildWorld([&](TestMwmBuilder & builder) { builder.Add(city); });

  SetViewport(m2::RectD(m2::PointD(-1.0, -1.0), m2::PointD(1.0, 1.0)));
  {
    TRules const rules{ExactMatch(id, city)};
    TEST(ResultsMatch("moscow", "en", rules), ());
  }

  CitiesBoundariesTable table(m_dataSource);
  TEST(table.Load(), ());
  TEST(table.Has(0 /* fid */), ());
  TEST(!table.Has(10 /* fid */), ());

  CitiesBoundariesTable::Boundaries boundaries;
  TEST(table.Get(0 /* fid */, boundaries), ());
  TEST(boundaries.HasPoint(m2::PointD(0, 0)), ());
  TEST(boundaries.HasPoint(m2::PointD(0.5, 0)), ());
  TEST(boundaries.HasPoint(m2::PointD(0.5, 0.5)), ());
  TEST(boundaries.HasPoint(m2::PointD(0, 0.5)), ());
  TEST(boundaries.HasPoint(m2::PointD(0.25, 0.25)), ());

  TEST(!boundaries.HasPoint(m2::PointD(0.6, 0.6)), ());
  TEST(!boundaries.HasPoint(m2::PointD(-1, 0.5)), ());
}

UNIT_CLASS_TEST(ProcessorTest, CityBoundarySmoke)
{
  TestCity moscow(vector<m2::PointD>({m2::PointD(0, 0), m2::PointD(0.5, 0), m2::PointD(0.5, 0.5),
                                      m2::PointD(0, 0.5)}),
                  "Москва", "ru", 100 /* rank */);
  TestCity khimki(vector<m2::PointD>({m2::PointD(0.25, 0.5), m2::PointD(0.5, 0.5),
                                      m2::PointD(0.5, 0.75), m2::PointD(0.25, 0.75)}),
                  "Химки", "ru", 50 /* rank */);

  TestPOI cafeMoscow(m2::PointD(0.49, 0.49), "Москвичка", "ru");
  cafeMoscow.SetTypes({{"amenity", "cafe"}, {"internet_access", "wlan"}});

  TestPOI cafeKhimki(m2::PointD(0.49, 0.51), "Химичка", "ru");
  cafeKhimki.SetTypes({{"amenity", "cafe"}, {"internet_access", "wlan"}});

  BuildWorld([&](TestMwmBuilder & builder) {
    builder.Add(moscow);
    builder.Add(khimki);
  });

  auto countryId = BuildCountry("Россия" /* countryName */, [&](TestMwmBuilder & builder) {
    builder.Add(cafeMoscow);
    builder.Add(cafeKhimki);
  });

  SetViewport(m2::RectD(m2::PointD(-1.0, -1.0), m2::PointD(1.0, 1.0)));

  {
    auto request = MakeRequest("кафе", "ru");
    auto const & results = request->Results();

    TRules rules{ExactMatch(countryId, cafeMoscow), ExactMatch(countryId, cafeKhimki)};
    TEST(ResultsMatch(results, rules), ());

    for (auto const & result : results)
    {
      if (ResultMatches(result, ExactMatch(countryId, cafeMoscow)))
      {
        TEST_EQUAL(result.GetAddress(), "Москва, Россия", ());
      }
      else
      {
        TEST(ResultMatches(result, ExactMatch(countryId, cafeKhimki)), ());
        TEST_EQUAL(result.GetAddress(), "Химки, Россия", ());
      }
    }
  }
}

// // Tests for the non-strict aspects of retrieval.
// // Currently, the only possible non-strictness is that
// // some tokens in the query may be ignored,
// // which results in a pruned parse tree for the query.
// UNIT_CLASS_TEST(ProcessorTest, RelaxedRetrieval)
// {
//   string const countryName = "Wonderland";
//   TestCountry country(m2::PointD(10.0, 10.0), countryName, "en");

//   TestCity city({{-10.0, -10.0}, {10.0, -10.0}, {10.0, 10.0}, {-10.0, 10.0}} /* boundary */,
//                 "Sick City", "en", 255 /* rank */);

//   TestStreet street(vector<m2::PointD>{m2::PointD(-1.0, 0.0), m2::PointD(1.0, 0.0)}, "Queer Street",
//                     "en");
//   TestBuilding building0(m2::PointD(-1.0, 0.0), "" /* name */, "0", street, "en");
//   TestBuilding building1(m2::PointD(1.0, 0.0), "", "1", street, "en");
//   TestBuilding building2(m2::PointD(2.0, 0.0), "named building", "" /* house number */, "en");
//   TestBuilding building3(m2::PointD(3.0, 0.0), "named building", "", "en");

//   TestPOI poi0(m2::PointD(-1.0, 0.0), "Farmacia de guardia", "en");
//   poi0.SetTypes({{"amenity", "pharmacy"}});

//   // A poi inside building2.
//   TestPOI poi2(m2::PointD(2.0, 0.0), "Post box", "en");
//   poi2.SetTypes({{"amenity", "post_box"}});

//   auto countryId = BuildCountry(countryName, [&](TestMwmBuilder & builder) {
//     builder.Add(street);
//     builder.Add(building0);
//     builder.Add(building1);
//     builder.Add(poi0);
//   });
//   RegisterCountry(countryName, m2::RectD(m2::PointD(-10.0, -10.0), m2::PointD(10.0, 10.0)));

//   auto worldId = BuildWorld([&](TestMwmBuilder & builder) {
//     builder.Add(country);
//     builder.Add(city);
//   });

//   {
//     TRules rulesStrict = {ExactMatch(countryId, building0)};
//     TRules rulesRelaxed = {ExactMatch(countryId, street)};

//     // "street" instead of "street-building"
//     TEST(ResultsMatch("queer street 0 ", rulesStrict), ());
//     TEST(ResultsMatch("queer street ", rulesRelaxed), ());
//     TEST(ResultsMatch("queer street 2 ", rulesRelaxed), ());
//   }

//   {
//     TRules rulesStrict = {ExactMatch(countryId, poi0), ExactMatch(countryId, street)};
//     TRules rulesRelaxed = {ExactMatch(countryId, street)};

//     // "country-city-street" instead of "country-city-street-poi"
//     TEST(ResultsMatch("wonderland sick city queer street pharmacy ", rulesStrict), ());
//     TEST(ResultsMatch("wonderland sick city queer street school ", rulesRelaxed), ());
//   }

//   {
//     TRules rulesStrict = {ExactMatch(countryId, street)};
//     TRules rulesRelaxed = {};

//     // Cities and larger toponyms should not be relaxed.
//     // "city" instead of "city-street"
//     TEST(ResultsMatch("sick city queer street ", rulesStrict), ());
//     TEST(ResultsMatch("sick city sick street ", rulesRelaxed), ());
//   }

//   {
//     TRules rulesStrict = {ExactMatch(countryId, street)};
//     TRules rulesRelaxed = {};

//     // Should not be relaxed.
//     // "country-city" instead of "country-city-street"
//     TEST(ResultsMatch("wonderland sick city queer street ", rulesStrict), ());
//     TEST(ResultsMatch("wonderland sick city other street ", rulesRelaxed), ());
//   }

//   {
//     TRules rulesStrict = {ExactMatch(countryId, poi0)};
//     TRules rulesRelaxed = {};

//     // Should not be relaxed.
//     // "city" instead of "city-poi"
//     TEST(ResultsMatch("sick city pharmacy ", rulesStrict), ());
//     TEST(ResultsMatch("sick city library ", rulesRelaxed), ());
//   }
// }

UNIT_CLASS_TEST(ProcessorTest, PathsThroughLayers)
{
  string const countryName = "Science Country";

  TestCountry scienceCountry(m2::PointD(0.0, 0.0), countryName, "en");

  TestCity mathTown(vector<m2::PointD>({m2::PointD(-100.0, -100.0), m2::PointD(100.0, -100.0),
                                        m2::PointD(100.0, 100.0), m2::PointD(-100.0, 100.0)}),
                    "Math Town", "en", 100 /* rank */);

  TestStreet computingStreet(
      vector<m2::PointD>{m2::PointD{-16.0, -16.0}, m2::PointD(0.0, 0.0), m2::PointD(16.0, 16.0)},
      "Computing street", "en");

  TestBuilding statisticalLearningBuilding(m2::PointD(8.0, 8.0), "Statistical Learning, Inc.", "0",
                                           computingStreet, "en");

  TestPOI reinforcementCafe(m2::PointD(8.0, 8.0), "Trattoria Reinforcemento", "en");
  reinforcementCafe.SetTypes({{"amenity", "cafe"}});

  BuildWorld([&](TestMwmBuilder & builder) {
    builder.Add(scienceCountry);
    builder.Add(mathTown);
  });
  RegisterCountry(countryName, m2::RectD(m2::PointD(-100.0, -100.0), m2::PointD(100.0, 100.0)));

  auto countryId = BuildCountry(countryName, [&](TestMwmBuilder & builder) {
    builder.Add(computingStreet);
    builder.Add(statisticalLearningBuilding);
    builder.Add(reinforcementCafe);
  });

  SetViewport(m2::RectD(m2::PointD(-100.0, -100.0), m2::PointD(100.0, 100.0)));

  for (auto const mode :
       {FeaturesLayerPathFinder::MODE_AUTO, FeaturesLayerPathFinder::MODE_TOP_DOWN,
        FeaturesLayerPathFinder::MODE_BOTTOM_UP})
  {
    FeaturesLayerPathFinder::SetModeForTesting(mode);
    SCOPE_GUARD(rollbackMode, [&] {
      FeaturesLayerPathFinder::SetModeForTesting(FeaturesLayerPathFinder::MODE_AUTO);
    });

    auto const ruleStreet = ExactMatch(countryId, computingStreet);
    auto const ruleBuilding = ExactMatch(countryId, statisticalLearningBuilding);
    auto const rulePoi = ExactMatch(countryId, reinforcementCafe);

    // POI-BUILDING-STREET
    TEST(ResultsMatch("computing street statistical learning cafe ", {rulePoi}), ());
    TEST(ResultsMatch("computing street 0 cafe ", {rulePoi}), ());

    // POI-BUILDING
    TEST(ResultsMatch("statistical learning cafe ", {rulePoi}), ());
    TEST(ResultsMatch("0 cafe ", {rulePoi}), ());

    // POI-STREET
    TEST(ResultsMatch("computing street cafe ", {rulePoi}), ());

    // BUILDING-STREET
    TEST(ResultsMatch("computing street statistical learning ", {ruleBuilding}), ());
    TEST(ResultsMatch("computing street 0 ", {ruleBuilding}), ());

    // POI
    TEST(ResultsMatch("cafe ", {rulePoi}), ());

    // BUILDING
    TEST(ResultsMatch("statistical learning ", {ruleBuilding}), ());
    // Cannot find anything only by a house number.
    TEST(ResultsMatch("0 ", TRules{}), ());

    // STREET
    TEST(ResultsMatch("computing street ", {ruleStreet}), ());
  }
}

UNIT_CLASS_TEST(ProcessorTest, SelectProperName)
{
  string const countryName = "Wonderland";

  auto testLanguage = [&](string language, string expectedRes) {
    auto request = MakeRequest("cafe", language);
    auto const & results = request->Results();
    TEST_EQUAL(results.size(), 1, (results));
    TEST_EQUAL(results[0].GetString(), expectedRes, (results));
  };

  TestMultilingualPOI cafe(
      m2::PointD(0.0, 0.0), "Default",
      {{"es", "Spanish"}, {"int_name", "International"}, {"fr", "French"}, {"ru", "Russian"}});
  cafe.SetTypes({{"amenity", "cafe"}});

  auto wonderlandId = BuildCountry(countryName, [&](TestMwmBuilder & builder) {
    builder.Add(cafe);
    builder.SetMwmLanguages({"it", "fr"});
  });

  SetViewport(m2::RectD(-1, -1, 1, 1));

  // Language priorities:
  // - device language
  // - input language
  // - default if mwm has device or input region
  // - english and international
  // - default

  // Device language is English by default.

  // No English(device) or Italian(query) name present but mwm has Italian region.
  testLanguage("it", "Default");

  // No English(device) name present. Mwm has French region but we should prefer explicit French
  // name.
  testLanguage("fr", "French");

  // No English(device) name present. Use query language.
  testLanguage("es", "Spanish");

  // No English(device) or German(query) names present. Mwm does not have German region. We should
  // use international name.
  testLanguage("de", "International");

  // Set Russian as device language.
  m_engine.SetLocale("ru");

  // Device language(Russian) is present and preferred for all queries.
  testLanguage("it", "Russian");
  testLanguage("fr", "Russian");
  testLanguage("es", "Russian");
  testLanguage("de", "Russian");
}

UNIT_CLASS_TEST(ProcessorTest, CuisineTest)
{
  string const countryName = "Wonderland";

  TestPOI vegan(m2::PointD(1.0, 1.0), "Useless name", "en");
  vegan.SetTypes({{"amenity", "cafe"}, {"cuisine", "vegan"}});

  TestPOI pizza(m2::PointD(1.0, 1.0), "Useless name", "en");
  pizza.SetTypes({{"amenity", "bar"}, {"cuisine", "pizza"}});

  auto countryId = BuildCountry(countryName, [&](TestMwmBuilder & builder) {
    builder.Add(vegan);
    builder.Add(pizza);
  });

  SetViewport(m2::RectD(-1, -1, 1, 1));

  {
    TRules rules{ExactMatch(countryId, vegan)};
    TEST(ResultsMatch("vegan ", "en", rules), ());
  }

  {
    TRules rules{ExactMatch(countryId, pizza)};
    TEST(ResultsMatch("pizza ", "en", rules), ());
    TEST(ResultsMatch("pizzeria ", "en", rules), ());
  }
}

UNIT_CLASS_TEST(ProcessorTest, CuisineMetadataTest)
{
  string const countryName = "Wonderland";

  TestCafeWithCuisine kebab(m2::PointD(1.0, 1.0), "Useless name", "en", "kebab");
  TestCafeWithCuisine tapas(m2::PointD(1.0, 1.0), "Useless name", "en", "tapas");

  // Metadata is supported for old maps only.
  SetMwmVersion(180801);

  auto countryId = BuildCountry(countryName, [&](TestMwmBuilder & builder) {
    builder.Add(kebab);
    builder.Add(tapas);
  });

  SetViewport(m2::RectD(-1, -1, 1, 1));

  {
    TRules rules{ExactMatch(countryId, kebab)};
    TEST(ResultsMatch("kebab ", "en", rules), ());
  }

  {
    TRules rules{ExactMatch(countryId, tapas)};
    TEST(ResultsMatch("tapas ", "en", rules), ());
  }
}

UNIT_CLASS_TEST(ProcessorTest, AirportTest)
{
  string const countryName = "Wonderland";

  TestAirport vko(m2::PointD(1.0, 1.0), "Useless name", "en", "VKO");
  TestAirport svo(m2::PointD(1.0, 1.0), "Useless name", "en", "SVO");

  auto countryId = BuildCountry(countryName, [&](TestMwmBuilder & builder) {
    builder.Add(vko);
    builder.Add(svo);
  });

  SetViewport(m2::RectD(-1, -1, 1, 1));

  {
    TRules rules{ExactMatch(countryId, vko)};
    TEST(ResultsMatch("vko ", "en", rules), ());
  }

  {
    TRules rules{ExactMatch(countryId, svo)};
    TEST(ResultsMatch("svo ", "en", rules), ());
  }
}

UNIT_CLASS_TEST(ProcessorTest, OperatorTest)
{
  string const countryName = "Wonderland";

  TestATM sber(m2::PointD(1.0, 1.0), "Sberbank", "en");
  TestATM alfa(m2::PointD(1.0, 1.0), "Alfa bank", "en");

  auto countryId = BuildCountry(countryName, [&](TestMwmBuilder & builder) {
    builder.Add(sber);
    builder.Add(alfa);
  });

  SetViewport(m2::RectD(-1, -1, 1, 1));

  {
    TRules rules{ExactMatch(countryId, sber)};
    TEST(ResultsMatch("sberbank ", "en", rules), ());
    TEST(ResultsMatch("sberbank atm ", "en", rules), ());
    TEST(ResultsMatch("atm sberbank ", "en", rules), ());
  }

  {
    TRules rules{ExactMatch(countryId, alfa)};
    TEST(ResultsMatch("alfa bank ", "en", rules), ());
    TEST(ResultsMatch("alfa bank atm ", "en", rules), ());
    TEST(ResultsMatch("alfa atm ", "en", rules), ());
    TEST(ResultsMatch("atm alfa bank ", "en", rules), ());
    TEST(ResultsMatch("atm alfa ", "en", rules), ());
  }
}

UNIT_CLASS_TEST(ProcessorTest, BrandTest)
{
  string const countryName = "Wonderland";

  TestBrandFeature mac(m2::PointD(1.0, 1.0), "mcdonalds", "en");
  TestBrandFeature sw(m2::PointD(1.0, 1.0), "subway", "en");

  auto countryId = BuildCountry(countryName, [&](TestMwmBuilder & builder) {
    builder.Add(mac);
    builder.Add(sw);
  });

  SetViewport(m2::RectD(-1, -1, 1, 1));

  {
    TRules rules{ExactMatch(countryId, mac)};
    TEST(ResultsMatch("McDonald's", "en", rules), ());
    TEST(ResultsMatch("Mc Donalds", "en", rules), ());
    TEST(ResultsMatch("МакДональд'с", "ru", rules), ());
    TEST(ResultsMatch("Мак Доналдс", "ru", rules), ());
    TEST(ResultsMatch("マクドナルド", "ja", rules), ());
  }

  {
    TRules rules{ExactMatch(countryId, sw)};
    TEST(ResultsMatch("Subway", "en", rules), ());
    TEST(ResultsMatch("Сабвэй", "ru", rules), ());
    TEST(ResultsMatch("サブウェイ", "ja", rules), ());
  }
}
}  // namespace
}  // namespace search
