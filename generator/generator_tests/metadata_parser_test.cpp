#include "testing/testing.hpp"

#include "generator/osm2meta.hpp"

#include "indexer/classificator_loader.hpp"

#include "coding/writer.hpp"
#include "coding/reader.hpp"

#include "base/logging.hpp"

#include "std/target_os.hpp"

using feature::Metadata;

UNIT_TEST(Metadata_ValidateAndFormat_stars)
{
  FeatureParams params;
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

UNIT_TEST(Metadata_ValidateAndFormat_operator)
{
  classificator::Load();
  Classificator const & c = classif();
  uint32_t const type_atm = c.GetTypeByPath({ "amenity", "atm" });
  uint32_t const type_fuel = c.GetTypeByPath({ "amenity", "fuel" });

  FeatureParams params;
  MetadataTagProcessor p(params);
  Metadata & md = params.GetMetadata();

  // Ignore tag 'operator' if feature have inappropriate type.
  p("operator", "Some");
  TEST(md.Empty(), ());

  params.SetType(type_atm);
  p("operator", "Some");
  TEST_EQUAL(md.Get(Metadata::FMD_OPERATOR), "Some", ());
  md.Drop(Metadata::FMD_OPERATOR);

  params.SetType(type_fuel);
  p("operator", "Some");
  TEST_EQUAL(md.Get(Metadata::FMD_OPERATOR), "Some", ());
  md.Drop(Metadata::FMD_OPERATOR);

  params.SetType(type_atm);
  params.AddType(type_fuel);
  p("operator", "Some");
  TEST_EQUAL(md.Get(Metadata::FMD_OPERATOR), "Some", ());
  md.Drop(Metadata::FMD_OPERATOR);
}

UNIT_TEST(Metadata_ValidateAndFormat_height)
{
  FeatureParams params;
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

  FeatureParams params;
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
