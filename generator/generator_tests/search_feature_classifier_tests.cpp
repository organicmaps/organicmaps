#include "testing/testing.hpp"

#include "generator/feature_builder.hpp"
#include "generator/generator_tests_support/test_with_classificator.hpp"
#include "generator/search_index_builder.hpp"

#include "indexer/classificator.hpp"
#include "indexer/scales.hpp"

#include <initializer_list>
#include <string>
#include <string_view>

namespace search_feature_classifier_tests
{
using generator::tests_support::TestWithClassificator;
using Group = feature::DataHeader::FeatureGroup;
using feature::GeomType;

feature::FeatureBuilder MakeFeature(std::initializer_list<base::StringIL> const & paths,
                                    GeomType geometry = GeomType::Point, std::string_view name = {},
                                    std::string_view houseNumber = {})
{
  feature::FeatureBuilder fb;
  if (geometry == GeomType::Point)
    fb.SetCenter({0, 0});
  else if (geometry == GeomType::Line)
  {
    fb.AssignPoints({{0, 0}, {1, 1}});
    fb.SetLinear();
  }
  else
  {
    fb.AssignArea({{0, 0}, {0, 1}, {1, 1}, {1, 0}, {0, 0}}, {});
    fb.SetArea();
  }

  for (auto const & path : paths)
    fb.AddType(classif().GetTypeByPath(path));
  CHECK(fb.GetParams().FinishAddingTypes(), ());

  if (!name.empty())
    fb.SetName(StringUtf8Multilang::kDefaultCode, name);
  if (!houseNumber.empty())
    fb.GetParams().AddHouseNumber(std::string(houseNumber));
  CHECK(fb.PreSerialize(), ());
  return fb;
}

UNIT_CLASS_TEST(TestWithClassificator, SearchFeatureClassifier_AddressesAndNames)
{
  indexer::SearchFeatureClassifier const classifier({0, scales::GetUpperScale()});
  auto const check = [&classifier](feature::FeatureBuilder const & fb, Group expected)
  { TEST_EQUAL(static_cast<int>(classifier.GetFeatureGroup(fb)), static_cast<int>(expected), (fb)); };

  check(MakeFeature({{"building"}}, GeomType::Area), Group::Other);
  check(MakeFeature({{"building"}}, GeomType::Area, {}, "12"), Group::Pois);
  check(MakeFeature({{"building"}}, GeomType::Area, "The Lodge"), Group::Pois);
  check(MakeFeature({{"building"}}, GeomType::Area, "The Lodge", "12"), Group::Pois);
  check(MakeFeature({{"building:part"}}, GeomType::Area), Group::Other);
  check(MakeFeature({{"building:part"}}, GeomType::Area, {}, "12"), Group::Pois);
  check(MakeFeature({{"amenity", "cafe"}}, GeomType::Point, {}, "12"), Group::Pois);

  auto interpolation = MakeFeature({{"addr:interpolation", "even"}}, GeomType::Line);
  interpolation.GetParams().ref = "12:40";
  check(interpolation, Group::Pois);
  check(MakeFeature({{"addr:interpolation"}}, GeomType::Point), Group::Other);

  check(MakeFeature({{"entrance"}}, GeomType::Point, "3", "12"), Group::Pois);
  check(MakeFeature({{"entrance"}}, GeomType::Point, "3"), Group::Other);
  check(MakeFeature({{"entrance"}}, GeomType::Point, "Main entrance"), Group::Pois);
}

UNIT_CLASS_TEST(TestWithClassificator, SearchFeatureClassifier_NaturalFeaturesAndPrecedence)
{
  indexer::SearchFeatureClassifier const classifier({0, scales::GetUpperScale()});
  auto const check = [&classifier](feature::FeatureBuilder const & fb, Group expected)
  { TEST_EQUAL(static_cast<int>(classifier.GetFeatureGroup(fb)), static_cast<int>(expected), (fb)); };

  check(MakeFeature({{"landuse", "forest"}}, GeomType::Area), Group::Other);
  check(MakeFeature({{"landuse", "forest"}}, GeomType::Area, "Silver Woods"), Group::Pois);
  check(MakeFeature({{"natural", "water", "lake"}}, GeomType::Area), Group::Other);
  check(MakeFeature({{"natural", "water", "lake"}}, GeomType::Area, "Silver Lake"), Group::Pois);
  check(MakeFeature({{"natural", "spring"}}), Group::Pois);

  auto scrub = MakeFeature({{"natural", "scrub"}}, GeomType::Area, "...");
  check(scrub, Group::Other);
  scrub.SetName(StringUtf8Multilang::kAltNameCode, "The Grove");
  check(scrub, Group::Pois);

  check(MakeFeature({{"highway", "residential"}}, GeomType::Line), Group::Streets);
  check(MakeFeature({{"highway", "residential"}}), Group::Other);
  check(MakeFeature({{"place", "square"}}), Group::Streets);
  check(MakeFeature({{"place", "city"}, {"amenity", "cafe"}}), Group::Places);
  check(MakeFeature({{"landuse", "residential"}}, GeomType::Area), Group::Other);
  check(MakeFeature({{"landuse", "residential"}}, GeomType::Area, "Garden Village"), Group::Places);
  check(MakeFeature({{"place", "city"}}), Group::Other);
  check(MakeFeature({{"place", "city"}}, GeomType::Point, "City"), Group::Places);
  check(MakeFeature({{"place", "suburb"}}), Group::Other);
  check(MakeFeature({{"place", "suburb"}}, GeomType::Point, "Suburb"), Group::Places);
  check(MakeFeature({{"place", "locality"}}), Group::Other);
  check(MakeFeature({{"place", "locality"}}, GeomType::Point, "Locality"), Group::Places);
  check(MakeFeature({{"place", "island"}}, GeomType::Area), Group::Other);
  check(MakeFeature({{"place", "island"}}, GeomType::Area, "Island"), Group::Places);
  check(MakeFeature({{"place", "region"}}), Group::Other);
  check(MakeFeature({{"place", "region"}}, GeomType::Point, "Region"), Group::Places);
  check(MakeFeature({{"highway", "residential"}, {"amenity", "cafe"}}, GeomType::Line), Group::Streets);
}

UNIT_CLASS_TEST(TestWithClassificator, SearchFeatureClassifier_MetadataNames)
{
  indexer::SearchFeatureClassifier const classifier({0, scales::GetUpperScale()});
  auto fb = MakeFeature({{"landuse", "forest"}}, GeomType::Area);
  fb.GetMetadata().Set(feature::Metadata::FMD_POSTCODE, "12345");
  TEST_EQUAL(static_cast<int>(classifier.GetFeatureGroup(fb)), static_cast<int>(Group::Other), ());
  fb.GetParams().AddHouseNumber("12");
  TEST_EQUAL(static_cast<int>(classifier.GetFeatureGroup(fb)), static_cast<int>(Group::Pois), ());

  for (auto const metadata : {feature::Metadata::FMD_OPERATOR, feature::Metadata::FMD_BRAND})
  {
    auto named = MakeFeature({{"landuse", "forest"}}, GeomType::Area);
    TEST_EQUAL(static_cast<int>(classifier.GetFeatureGroup(named)), static_cast<int>(Group::Other), ());
    named.GetMetadata().Set(metadata, "Woodland Trust");
    TEST_EQUAL(static_cast<int>(classifier.GetFeatureGroup(named)), static_cast<int>(Group::Pois), (metadata));
    named.GetMetadata().Drop(metadata);
    TEST_EQUAL(static_cast<int>(classifier.GetFeatureGroup(named)), static_cast<int>(Group::Other), (metadata));
  }

  indexer::SearchFeatureClassifier const lowZoomClassifier({0, 0});
  auto airport = MakeFeature({{"aeroway", "aerodrome"}}, GeomType::Area);
  TEST_EQUAL(static_cast<int>(lowZoomClassifier.GetFeatureGroup(airport)), static_cast<int>(Group::Other), ());
  airport.GetMetadata().Set(feature::Metadata::FMD_AIRPORT_IATA, "SFO");
  TEST_EQUAL(static_cast<int>(lowZoomClassifier.GetFeatureGroup(airport)), static_cast<int>(Group::Pois), ());

  auto isoline = MakeFeature({{"isoline"}, {"amenity", "cafe"}}, GeomType::Line, "Contour");
  isoline.GetMetadata().Set(feature::Metadata::FMD_OPERATOR, "Operator");
  TEST_EQUAL(static_cast<int>(classifier.GetFeatureGroup(isoline)), static_cast<int>(Group::Other), ());
}

UNIT_CLASS_TEST(TestWithClassificator, SearchFeatureClassifier_CategoryMatching)
{
  indexer::SearchFeatureClassifier const classifier({0, scales::GetUpperScale()});
  auto const building = MakeFeature({{"building"}}, GeomType::Area);
  TEST(classifier.GetCategoryTypes(building.GetTypesHolder(), false).empty(), ());
  TEST_EQUAL(classifier.GetCategoryTypes(building.GetTypesHolder(), true).size(), 1, ());

  auto const statue = MakeFeature({{"historic", "memorial", "statue"}});
  auto const categories = classifier.GetCategoryTypes(statue.GetTypesHolder(), false);
  TEST_EQUAL(categories.size(), 1, ());
  TEST_EQUAL(categories.front(), classif().GetTypeByPath({"historic", "memorial", "statue"}), ());

  auto const pharmacy = MakeFeature({{"amenity", "pharmacy"}});
  indexer::SearchFeatureClassifier const lowZoomClassifier({0, 0});
  TEST_EQUAL(classifier.GetCategoryTypes(pharmacy.GetTypesHolder(), false).size(), 1, ());
  TEST(lowZoomClassifier.GetCategoryTypes(pharmacy.GetTypesHolder(), false).empty(), ());
}
}  // namespace search_feature_classifier_tests
