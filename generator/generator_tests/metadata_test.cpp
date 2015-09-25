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
  FeatureParams params;
  MetadataTagProcessor p(params);
  string const lanaWoodUrlEncoded = "%D0%9B%D0%B0%D0%BD%D0%B0_%D0%92%D1%83%D0%B4";

  p("wikipedia", "ru:Лана Вуд");
  TEST_EQUAL(params.GetMetadata().Get(feature::Metadata::FMD_WIKIPEDIA), "ru:" + lanaWoodUrlEncoded, ("ru:"));
  params.GetMetadata().Drop(feature::Metadata::FMD_WIKIPEDIA);

  p("wikipedia", "https://ru.wikipedia.org/wiki/" + lanaWoodUrlEncoded);
  TEST_EQUAL(params.GetMetadata().Get(feature::Metadata::FMD_WIKIPEDIA), "ru:" + lanaWoodUrlEncoded, ("https:"));
  params.GetMetadata().Drop(feature::Metadata::FMD_WIKIPEDIA);

  p("wikipedia", "Test");
  TEST(params.GetMetadata().Empty(), ("Test"));

  p("wikipedia", "https://en.wikipedia.org/wiki/");
  TEST(params.GetMetadata().Empty(), ("Null wiki"));

  p("wikipedia", "http://ru.google.com/wiki/wutlol");
  TEST(params.GetMetadata().Empty(), ("Google"));
}

UNIT_TEST(Metadata_ReadWrite_Intermediate)
{
  FeatureParams params;
  FeatureParams params_test;
  MetadataTagProcessor p(params);

  vector<char> buffer;
  MemWriter<vector<char> > writer(buffer);

  p("stars", "3");
  p("phone", "+123456789");
  p("opening_hours", "24/7");
  p("cuisine", "regional");
  p("operator", "Unused");
  params.GetMetadata().Serialize(writer);

  MemReader reader(buffer.data(), buffer.size());
  ReaderSource<MemReader> src(reader);
  params_test.GetMetadata().Deserialize(src);

  TEST_EQUAL(params_test.GetMetadata().Get(feature::Metadata::FMD_STARS), "3", ())
  TEST_EQUAL(params_test.GetMetadata().Get(feature::Metadata::FMD_PHONE_NUMBER), "+123456789", ())
  TEST_EQUAL(params_test.GetMetadata().Get(feature::Metadata::FMD_OPEN_HOURS), "24/7", ())
  TEST_EQUAL(params_test.GetMetadata().Get(feature::Metadata::FMD_CUISINE), "regional", ())
  TEST(params_test.GetMetadata().Get(feature::Metadata::FMD_OPERATOR).empty(), ())
}

UNIT_TEST(Metadata_ReadWrite_MWM)
{
  FeatureParams params;
  FeatureParams params_test;
  MetadataTagProcessor p(params);

  vector<char> buffer;
  MemWriter<vector<char> > writer(buffer);

  p("stars", "3");
  p("phone", "+123456789");
  p("opening_hours", "24/7");
  p("cuisine", "regional");
  p("operator", "Unused");
  params.GetMetadata().SerializeToMWM(writer);

  MemReader reader(buffer.data(), buffer.size());
  ReaderSource<MemReader> src(reader);
  params_test.GetMetadata().DeserializeFromMWM(src);

  TEST_EQUAL(params_test.GetMetadata().Get(feature::Metadata::FMD_STARS), "3", ())
  TEST_EQUAL(params_test.GetMetadata().Get(feature::Metadata::FMD_PHONE_NUMBER), "+123456789", ())
  TEST_EQUAL(params_test.GetMetadata().Get(feature::Metadata::FMD_OPEN_HOURS), "24/7", ())
  TEST_EQUAL(params_test.GetMetadata().Get(feature::Metadata::FMD_CUISINE), "regional", ())
  TEST(params_test.GetMetadata().Get(feature::Metadata::FMD_OPERATOR).empty(), ())
}
