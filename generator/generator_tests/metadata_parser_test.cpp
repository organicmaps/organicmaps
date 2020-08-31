#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_with_classificator.hpp"
#include "generator/osm2meta.hpp"

#include "indexer/classificator.hpp"

#include "base/logging.hpp"

using namespace generator::tests_support;

using feature::Metadata;

UNIT_TEST(Metadata_ValidateAndFormat_stars)
{
  FeatureBuilderParams params;
  MetadataTagProcessor p(params);
  Metadata & md = params.GetMetadata();

  // Ignore incorrect values.
  p("stars", "0");
  TEST(md.Empty(), ());
  p("stars", "-1");
  TEST(md.Empty(), ());
  p("stars", "aasdasdas");
  TEST(md.Empty(), ());
  p("stars", "8");
  TEST(md.Empty(), ());
  p("stars", "10");
  TEST(md.Empty(), ());
  p("stars", "910");
  TEST(md.Empty(), ());
  p("stars", "100");
  TEST(md.Empty(), ());

  // Check correct values.
  p("stars", "1");
  TEST_EQUAL(md.Get(Metadata::FMD_STARS), "1", ());
  md.Drop(Metadata::FMD_STARS);

  p("stars", "2");
  TEST_EQUAL(md.Get(Metadata::FMD_STARS), "2", ());
  md.Drop(Metadata::FMD_STARS);

  p("stars", "3");
  TEST_EQUAL(md.Get(Metadata::FMD_STARS), "3", ());
  md.Drop(Metadata::FMD_STARS);

  p("stars", "4");
  TEST_EQUAL(md.Get(Metadata::FMD_STARS), "4", ());
  md.Drop(Metadata::FMD_STARS);

  p("stars", "5");
  TEST_EQUAL(md.Get(Metadata::FMD_STARS), "5", ());
  md.Drop(Metadata::FMD_STARS);

  p("stars", "6");
  TEST_EQUAL(md.Get(Metadata::FMD_STARS), "6", ());
  md.Drop(Metadata::FMD_STARS);

  p("stars", "7");
  TEST_EQUAL(md.Get(Metadata::FMD_STARS), "7", ());
  md.Drop(Metadata::FMD_STARS);

  // Check almost correct values.
  p("stars", "4+");
  TEST_EQUAL(md.Get(Metadata::FMD_STARS), "4", ());
  md.Drop(Metadata::FMD_STARS);

  p("stars", "5s");
  TEST_EQUAL(md.Get(Metadata::FMD_STARS), "5", ());
  md.Drop(Metadata::FMD_STARS);

}

UNIT_CLASS_TEST(TestWithClassificator, Metadata_ValidateAndFormat_operator)
{
  uint32_t const typeAtm = classif().GetTypeByPath({"amenity", "atm"});
  uint32_t const typeFuel = classif().GetTypeByPath({"amenity", "fuel"});
  uint32_t const typeCarSharing = classif().GetTypeByPath({"amenity", "car_sharing"});
  uint32_t const typeCarRental = classif().GetTypeByPath({"amenity", "car_rental"});

  FeatureBuilderParams params;
  MetadataTagProcessor p(params);
  Metadata & md = params.GetMetadata();

  // Ignore tag 'operator' if feature have inappropriate type.
  p("operator", "Some");
  TEST(md.Empty(), ());

  params.SetType(typeAtm);
  p("operator", "Some");
  TEST_EQUAL(md.Get(Metadata::FMD_OPERATOR), "Some", ());
  md.Drop(Metadata::FMD_OPERATOR);

  params.SetType(typeFuel);
  p("operator", "Some");
  TEST_EQUAL(md.Get(Metadata::FMD_OPERATOR), "Some", ());
  md.Drop(Metadata::FMD_OPERATOR);

  params.SetType(typeCarSharing);
  params.AddType(typeCarRental);
  p("operator", "Some");
  TEST_EQUAL(md.Get(Metadata::FMD_OPERATOR), "Some", ());
  md.Drop(Metadata::FMD_OPERATOR);
}

UNIT_TEST(Metadata_ValidateAndFormat_height)
{
  FeatureBuilderParams params;
  MetadataTagProcessor p(params);
  Metadata & md = params.GetMetadata();

  p("height", "0");
  TEST(md.Empty(), ());

  p("height", "0,0000");
  TEST(md.Empty(), ());

  p("height", "0.0");
  TEST(md.Empty(), ());

  p("height", "123");
  TEST_EQUAL(md.Get(Metadata::FMD_HEIGHT), "123", ());
  md.Drop(Metadata::FMD_HEIGHT);

  p("height", "123.2");
  TEST_EQUAL(md.Get(Metadata::FMD_HEIGHT), "123.2", ());
  md.Drop(Metadata::FMD_HEIGHT);

  p("height", "2 m");
  TEST_EQUAL(md.Get(Metadata::FMD_HEIGHT), "2", ());
  md.Drop(Metadata::FMD_HEIGHT);

  p("height", "3-6");
  TEST_EQUAL(md.Get(Metadata::FMD_HEIGHT), "6", ());
}

UNIT_TEST(Metadata_ValidateAndFormat_wikipedia)
{
  char const * kWikiKey = "wikipedia";

  FeatureBuilderParams params;
  MetadataTagProcessor p(params);
  Metadata & md = params.GetMetadata();

#ifdef OMIM_OS_MOBILE
  #define WIKIHOST "m.wikipedia.org"
#else
  #define WIKIHOST "wikipedia.org"
#endif

  p(kWikiKey, "en:Bad %20Data");
  TEST_EQUAL(md.Get(Metadata::FMD_WIKIPEDIA), "en:Bad %20Data", ());
  TEST_EQUAL(md.GetWikiURL(), "https://en." WIKIHOST "/wiki/Bad_%2520Data", ());
  md.Drop(Metadata::FMD_WIKIPEDIA);

  p(kWikiKey, "ru:Тест_with % sign");
  TEST_EQUAL(md.Get(Metadata::FMD_WIKIPEDIA), "ru:Тест with % sign", ());
  TEST_EQUAL(md.GetWikiURL(), "https://ru." WIKIHOST "/wiki/Тест_with_%25_sign", ());
  md.Drop(Metadata::FMD_WIKIPEDIA);

  p(kWikiKey, "https://be-tarask.wikipedia.org/wiki/Вялікае_Княства_Літоўскае");
  TEST_EQUAL(md.Get(Metadata::FMD_WIKIPEDIA), "be-tarask:Вялікае Княства Літоўскае", ());
  TEST_EQUAL(md.GetWikiURL(), "https://be-tarask." WIKIHOST "/wiki/Вялікае_Княства_Літоўскае", ());
  md.Drop(Metadata::FMD_WIKIPEDIA);

  // Final link points to https and mobile version.
  p(kWikiKey, "http://en.wikipedia.org/wiki/A");
  TEST_EQUAL(md.Get(Metadata::FMD_WIKIPEDIA), "en:A", ());
  TEST_EQUAL(md.GetWikiURL(), "https://en." WIKIHOST "/wiki/A", ());
  md.Drop(Metadata::FMD_WIKIPEDIA);

  p(kWikiKey, "invalid_input_without_language_and_colon");
  TEST(md.Empty(), (md.Get(Metadata::FMD_WIKIPEDIA)));

  p(kWikiKey, "https://en.wikipedia.org/wiki/");
  TEST(md.Empty(), (md.Get(Metadata::FMD_WIKIPEDIA)));

  p(kWikiKey, "http://wikipedia.org/wiki/Article");
  TEST(md.Empty(), (md.Get(Metadata::FMD_WIKIPEDIA)));

  p(kWikiKey, "http://somesite.org");
  TEST(md.Empty(), (md.Get(Metadata::FMD_WIKIPEDIA)));

  p(kWikiKey, "http://www.spamsitewithaslash.com/");
  TEST(md.Empty(), (md.Get(Metadata::FMD_WIKIPEDIA)));

  p(kWikiKey, "http://.wikipedia.org/wiki/Article");
  TEST(md.Empty(), (md.Get(Metadata::FMD_WIKIPEDIA)));

  // Ignore incorrect prefixes.
  p(kWikiKey, "ht.tps://en.wikipedia.org/wiki/Whuh");
  TEST_EQUAL(md.Get(Metadata::FMD_WIKIPEDIA), "en:Whuh", ());
  md.Drop(Metadata::FMD_WIKIPEDIA);

  p(kWikiKey, "http://ru.google.com/wiki/wutlol");
  TEST(md.Empty(), ("Not a wikipedia site."));

#undef WIKIHOST
}

// Look at: https://wiki.openstreetmap.org/wiki/Key:duration for details
// about "duration" format.

UNIT_CLASS_TEST(TestWithClassificator, Metadata_ValidateAndFormat_duration)
{
  FeatureBuilderParams params;
  params.AddType(classif().GetTypeByPath({"route", "ferry"}));
  MetadataTagProcessor p(params);
  Metadata & md = params.GetMetadata();

  auto const test = [&](std::string const & osm, std::string const & expected) {
    p("duration", osm);

    if (expected.empty())
    {
      TEST(md.Empty(), ());
    }
    else
    {
      TEST_EQUAL(md.Get(Metadata::FMD_DURATION), expected, ());
      md.Drop(Metadata::FMD_DURATION);
    }
  };

  // "10" - 10 minutes ~ 0.16667 hours
  test("10", "0.16667");
  // 10:00 - 10 hours
  test("10:00", "10");
  test("QWE", "");
  // 1:1:1 - 1 hour + 1 minute + 1 second
  test("1:1:1", "1.0169");
  // 10 hours and 30 minutes
  test("10:30", "10.5");
  test("30", "0.5");
  test("60", "1");
  test("120", "2");
  test("35:10", "35.167");

  test("35::10", "");
  test("", "");
  test("0", "");
  test("asd", "");
  test("10 minutes", "");
  test("01:15 h", "");
  test("08:00;07:00;06:30", "");
  test("3-4 minutes", "");
  test("5:00 hours", "");
  test("12 min", "");

  // means 20 seconds
  test("PT20S", "0.0055556");
  // means 7 minutes
  test("PT7M", "0.11667");
  // means 10 minutes and 40 seconds
  test("PT10M40S", "0.17778");
  test("PT50M", "0.83333");
  // means 2 hours
  test("PT2H", "2");
  // means 7 hours and 50 minutes
  test("PT7H50M", "7.8333");
  test("PT60M", "1");
  test("PT15M", "0.25");

  // means 1000 years, but we don't support such duration.
  test("PT1000Y", "");
  test("PTPT", "");
  // means 4 day, but we don't support such duration.
  test("P4D", "");
  test("PT50:20", "");
}
