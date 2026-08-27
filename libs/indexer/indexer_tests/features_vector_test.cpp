#include "testing/testing.hpp"

#include "indexer/features_vector.hpp"
#include "indexer/route_relation.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include "defines.hpp"

#include <map>
#include <string>
#include <utility>

namespace features_vector_test
{
using namespace std;

// Postcodes with frequencies.
// One can easily get this list of frequencies with postcodes:
//
// bzip2 -dc data/minsk-pass.osm.bz2 | grep 'k="addr:postcode"' |
// sed 's/.*v="\([^"]*\)".*/\1/' |
// sort | uniq -c | awk '{ printf("{%s, %s},\n", $2, $1) }'
constexpr pair<int, int> kCodeFreq[] = {{220000, 2},   {220001, 2},  {220004, 9}, {220006, 155}, {220007, 271},
                                        {220010, 5},   {220011, 1},  {220014, 3}, {220030, 271}, {220033, 7},
                                        {220036, 203}, {220039, 15}, {220048, 1}, {220050, 4},   {220060, 1},
                                        {220069, 7},   {220073, 1},  {220089, 1}, {220121, 1},   {721816, 1}};

UNIT_TEST(FeaturesVectorTest_ParseMetadata)
{
  string const kCountryName = "minsk-pass";

  map<string, int> expected;
  for (auto const & p : kCodeFreq)
    expected[strings::to_string(p.first)] = p.second;

  auto const path = base::JoinPath(GetPlatform().ResourcesDir(), kCountryName + DATA_FILE_EXTENSION);
  FeaturesVectorTest features(path);

  map<string, int> actual;
  features.GetVector().ForEach([&](FeatureType & ft, uint32_t index)
  {
    string const postcode(ft.GetMetadata(feature::Metadata::FMD_POSTCODE));
    if (!postcode.empty())
      ++actual[postcode];
  });
  TEST_EQUAL(expected, actual, ());
}

UNIT_TEST(FeaturesVectorTest_ParseRelation)
{
  auto const path = base::JoinPath(GetPlatform().ResourcesDir(), "minsk-pass" DATA_FILE_EXTENSION);
  FeaturesVectorTest features(path);
  auto const relation = features.GetVector().GetRelation(2);

  TEST(relation.GetType() == feature::RouteRelationBase::Type::Subway, ());
  TEST_EQUAL(relation.GetRef(), "1", ());
  TEST_EQUAL(relation.GetMembers().size(), 2, ());
}
}  // namespace features_vector_test
