#include "testing/testing.hpp"
#include "base/logging.hpp"

#include "indexer/classificator_loader.hpp"
#include "generator/osm2meta.hpp"
#include "coding/writer.hpp"
#include "coding/reader.hpp"

UNIT_TEST(Metadata_ValidateAndFormat_stars)
{
  FeatureParams params;
  MetadataTagProcessor p(params);

  // ignore incorrect values
  p("stars", "0");
  TEST(params.GetMetadata().Empty(), ());
  p("stars", "-1");
  TEST(params.GetMetadata().Empty(), ());
  p("stars", "aasdasdas");
  TEST(params.GetMetadata().Empty(), ());
  p("stars", "8");
  TEST(params.GetMetadata().Empty(), ());
  p("stars", "10");
  TEST(params.GetMetadata().Empty(), ());
  p("stars", "910");
  TEST(params.GetMetadata().Empty(), ());
  p("stars", "100");
  TEST(params.GetMetadata().Empty(), ());

  // check correct values
  p("stars", "1");
  TEST_EQUAL(params.GetMetadata().Get(feature::Metadata::FMD_STARS), "1", ())
  params.GetMetadata().Drop(feature::Metadata::FMD_STARS);

  p("stars", "2");
  TEST_EQUAL(params.GetMetadata().Get(feature::Metadata::FMD_STARS), "2", ())
  params.GetMetadata().Drop(feature::Metadata::FMD_STARS);

  p("stars", "3");
  TEST_EQUAL(params.GetMetadata().Get(feature::Metadata::FMD_STARS), "3", ())
  params.GetMetadata().Drop(feature::Metadata::FMD_STARS);

  p("stars", "4");
  TEST_EQUAL(params.GetMetadata().Get(feature::Metadata::FMD_STARS), "4", ())
  params.GetMetadata().Drop(feature::Metadata::FMD_STARS);

  p("stars", "5");
  TEST_EQUAL(params.GetMetadata().Get(feature::Metadata::FMD_STARS), "5", ())
  params.GetMetadata().Drop(feature::Metadata::FMD_STARS);

  p("stars", "6");
  TEST_EQUAL(params.GetMetadata().Get(feature::Metadata::FMD_STARS), "6", ())
  params.GetMetadata().Drop(feature::Metadata::FMD_STARS);

  p("stars", "7");
  TEST_EQUAL(params.GetMetadata().Get(feature::Metadata::FMD_STARS), "7", ())
  params.GetMetadata().Drop(feature::Metadata::FMD_STARS);

  // check almost correct values
  p("stars", "4+");
  TEST_EQUAL(params.GetMetadata().Get(feature::Metadata::FMD_STARS), "4", ())
  params.GetMetadata().Drop(feature::Metadata::FMD_STARS);

  p("stars", "5s");
  TEST_EQUAL(params.GetMetadata().Get(feature::Metadata::FMD_STARS), "5", ())
  params.GetMetadata().Drop(feature::Metadata::FMD_STARS);

}

UNIT_TEST(Metadata_ValidateAndFormat_operator)
{
  classificator::Load();
  Classificator const & c = classif();
  uint32_t const type_atm = c.GetTypeByPath({ "amenity", "atm" });
  uint32_t const type_fuel = c.GetTypeByPath({ "amenity", "fuel" });

  FeatureParams params;
  MetadataTagProcessor p(params);

  // ignore tag 'operator' if feature have inappropriate type
  p("operator", "Some");
  TEST(params.GetMetadata().Empty(), ());

  params.SetType(type_atm);
  p("operator", "Some");
  TEST_EQUAL(params.GetMetadata().Get(feature::Metadata::FMD_OPERATOR), "Some", ());
  params.GetMetadata().Drop(feature::Metadata::FMD_OPERATOR);

  params.SetType(type_fuel);
  p("operator", "Some");
  TEST_EQUAL(params.GetMetadata().Get(feature::Metadata::FMD_OPERATOR), "Some", ());
  params.GetMetadata().Drop(feature::Metadata::FMD_OPERATOR);

  params.SetType(type_atm);
  params.AddType(type_fuel);
  p("operator", "Some");
  TEST_EQUAL(params.GetMetadata().Get(feature::Metadata::FMD_OPERATOR), "Some", ());
  params.GetMetadata().Drop(feature::Metadata::FMD_OPERATOR);
}

UNIT_TEST(Metadata_ValidateAndFormat_ele)
{
  classificator::Load();
  Classificator const & c = classif();
  uint32_t const type_peak = c.GetTypeByPath({ "natural", "peak" });

  FeatureParams params;
  MetadataTagProcessor p(params);

  // ignore tag 'operator' if feature have inappropriate type
  p("ele", "123");
  TEST(params.GetMetadata().Empty(), ());

  params.SetType(type_peak);
  p("ele", "0");
  TEST(params.GetMetadata().Empty(), ());

  params.SetType(type_peak);
  p("ele", "0,0000");
  TEST(params.GetMetadata().Empty(), ());

  params.SetType(type_peak);
  p("ele", "0.0");
  TEST(params.GetMetadata().Empty(), ());

  params.SetType(type_peak);
  p("ele", "123");
  TEST_EQUAL(params.GetMetadata().Get(feature::Metadata::FMD_ELE), "123", ());
  params.GetMetadata().Drop(feature::Metadata::FMD_ELE);
}

UNIT_TEST(Metadata_ValidateAndFormat_wikipedia)
{
  using feature::Metadata;

  char const * kWikiKey = "wikipedia";

  FeatureParams params;
  MetadataTagProcessor p(params);

  p(kWikiKey, "en:Bad %20Data");
  TEST_EQUAL(params.GetMetadata().Get(Metadata::FMD_WIKIPEDIA), "en:Bad %20Data", ());
  TEST_EQUAL(params.GetMetadata().GetWikiURL(), "https://en.m.wikipedia.org/wiki/Bad_%2520Data", ());
  params.GetMetadata().Drop(Metadata::FMD_WIKIPEDIA);

  p(kWikiKey, "ru:Тест_with % sign");
  TEST_EQUAL(params.GetMetadata().Get(Metadata::FMD_WIKIPEDIA), "ru:Тест with % sign", ());
  TEST_EQUAL(params.GetMetadata().GetWikiURL(), "https://ru.m.wikipedia.org/wiki/Тест_with_%25_sign", ());
  params.GetMetadata().Drop(Metadata::FMD_WIKIPEDIA);

  p(kWikiKey, "https://be-tarask.wikipedia.org/wiki/Вялікае_Княства_Літоўскае");
  TEST_EQUAL(params.GetMetadata().Get(Metadata::FMD_WIKIPEDIA), "be-tarask:Вялікае Княства Літоўскае", ());
  TEST_EQUAL(params.GetMetadata().GetWikiURL(), "https://be-tarask.m.wikipedia.org/wiki/Вялікае_Княства_Літоўскае", ());
  params.GetMetadata().Drop(Metadata::FMD_WIKIPEDIA);

  // Final link points to https and mobile version.
  p(kWikiKey, "http://en.wikipedia.org/wiki/A");
  TEST_EQUAL(params.GetMetadata().Get(Metadata::FMD_WIKIPEDIA), "en:A", ());
  TEST_EQUAL(params.GetMetadata().GetWikiURL(), "https://en.m.wikipedia.org/wiki/A", ());
  params.GetMetadata().Drop(Metadata::FMD_WIKIPEDIA);

  p(kWikiKey, "invalid_input_without_language_and_colon");
  TEST(params.GetMetadata().Empty(), (params.GetMetadata().Get(Metadata::FMD_WIKIPEDIA)));

  p(kWikiKey, "https://en.wikipedia.org/wiki/");
  TEST(params.GetMetadata().Empty(), (params.GetMetadata().Get(Metadata::FMD_WIKIPEDIA)));

  p(kWikiKey, "http://wikipedia.org/wiki/Article");
  TEST(params.GetMetadata().Empty(), (params.GetMetadata().Get(Metadata::FMD_WIKIPEDIA)));

  p(kWikiKey, "http://somesite.org");
  TEST(params.GetMetadata().Empty(), (params.GetMetadata().Get(Metadata::FMD_WIKIPEDIA)));

  p(kWikiKey, "http://www.spamsitewithaslash.com/");
  TEST(params.GetMetadata().Empty(), (params.GetMetadata().Get(Metadata::FMD_WIKIPEDIA)));

  p(kWikiKey, "http://.wikipedia.org/wiki/Article");
  TEST(params.GetMetadata().Empty(), (params.GetMetadata().Get(Metadata::FMD_WIKIPEDIA)));

  // Ignore incorrect prefixes.
  p(kWikiKey, "ht.tps://en.wikipedia.org/wiki/Whuh");
  TEST_EQUAL(params.GetMetadata().Get(Metadata::FMD_WIKIPEDIA), "en:Whuh", ());
  params.GetMetadata().Drop(Metadata::FMD_WIKIPEDIA);

  p(kWikiKey, "http://ru.google.com/wiki/wutlol");
  TEST(params.GetMetadata().Empty(), ("Not a wikipedia site."));
}
