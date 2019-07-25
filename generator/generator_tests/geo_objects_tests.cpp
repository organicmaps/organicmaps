#include "testing/testing.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include "generator/feature_builder.hpp"
#include "generator/feature_generator.hpp"
#include "generator/generator_tests/common.hpp"
#include "generator/geo_objects/geo_object_info_getter.hpp"
#include "generator/geo_objects/geo_objects.hpp"

#include "indexer/classificator_loader.hpp"

#include <iostream>

using namespace generator_tests;
using namespace platform::tests_support;
using namespace generator;
using namespace generator::geo_objects;
using namespace feature;
using namespace base;

bool CheckWeGotExpectedIdsByPoint(m2::PointD const & point,
                                  std::vector<base::GeoObjectId> reference,
                                  indexer::GeoObjectsIndex<IndexReader> const & index)
{
  std::vector<base::GeoObjectId> test = GeoObjectInfoGetter::SearchObjectsInIndex(index, point);
  std::sort(test.begin(), test.end());
  std::sort(reference.begin(), reference.end());
  return test == reference;
}

UNIT_TEST(GenerateGeoObjects_AddNullBuildingGeometryForPointsWithAddressesInside)
{
  // Absolutely random region.
  std::shared_ptr<JsonValue> value = std::make_shared<JsonValue>(LoadFromString(
      R"({
               "type": "Feature",
               "geometry": {
                 "type": "Point",
                 "coordinates": [
                   -77.263927,
                   26.6210869
                 ]
               },
               "properties": {
                 "locales": {
                   "default": {
                     "address": {
                       "country": "Bahamas",
                       "region": "Central Abaco",
                       "locality": "Leisure Lee"
                     },
                     "name": "Leisure Lee"
                   },
                   "en": {
                     "address": {
                       "country": "Bahamas",
                       "region": "Central Abaco",
                       "locality": "Leisure Lee"
                     },
                     "name": "Leisure Lee"
                   },
                   "ru": {
                     "address": {
                       "country": "Багамы"
                     }
                   }
                 },
                 "rank": 4,
                 "dref": "C00000000039A088",
                 "code": "BS"
               }
             })"));

  auto regionGetter = [&value](auto && point) { return KeyValue{1, value}; };
  classificator::Load();

  ScopedFile const geoObjectsFeatures{"geo_objects_features.mwm", ScopedFile::Mode::DoNotCreate};
  ScopedFile const idsWithoutAddresses{"ids_without_addresses.txt", ScopedFile::Mode::DoNotCreate};
  ScopedFile const geoObjectsKeyValue{"geo_objects.jsonl", ScopedFile::Mode::DoNotCreate};

  std::vector<base::GeoObjectId> allIds;
  std::vector<OsmElementData> const osmElements{
      {1,
       {{"addr:housenumber", "39 с79"},
        {"addr:street", "Ленинградский проспект"},
        {"building", "yes"}},
       {{1.5, 1.5}},
       {}},
      {2,
       {{"building", "commercial"}, {"type", "multipolygon"}, {"name", "superbuilding"}},
       {{1, 1}, {4, 4}},
       {}},
      {3,
       {{"addr:housenumber", "39 с80"},
        {"addr:street", "Ленинградский проспект"},
        {"building", "yes"}},
       {{1.6, 1.6}},
       {}},
  };

  {
    FeaturesCollector collector(geoObjectsFeatures.GetFullPath());
    for (OsmElementData const & elementData : osmElements)
    {
      FeatureBuilder fb = FeatureBuilderFromOmsElementData(elementData);
      allIds.emplace_back(fb.GetMostGenericOsmId());
      TEST(fb.PreSerialize(), ());
      collector.Collect(fb);
    }
  }

  GeoObjectsGenerator geoObjectsGenerator{regionGetter,
                                          geoObjectsFeatures.GetFullPath(),
                                          idsWithoutAddresses.GetFullPath(),
                                          geoObjectsKeyValue.GetFullPath(),
                                          "*", /* allowAddresslessForCountries */
                                          false /* verbose */ ,
                                          1 /* threadsCount */};
  TEST(geoObjectsGenerator.GenerateGeoObjects(), ("Generate Geo Objects failed"));

  auto const geoObjectsIndex = MakeTempGeoObjectsIndex(geoObjectsFeatures.GetFullPath());
  TEST(geoObjectsIndex.has_value(), ("Temporary index build failed"));

  TEST(CheckWeGotExpectedIdsByPoint({1.5, 1.5}, allIds, *geoObjectsIndex), ());
  TEST(CheckWeGotExpectedIdsByPoint({2, 2}, allIds, *geoObjectsIndex), ());
  TEST(CheckWeGotExpectedIdsByPoint({4, 4}, allIds, *geoObjectsIndex), ());
}
