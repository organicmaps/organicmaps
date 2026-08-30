#include "testing/testing.hpp"

#include "indexer/data_source.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/mwm_set.hpp"

#include "platform/local_country_file.hpp"

#include <map>
#include <string>
#include <utility>

namespace features_vector_test
{
using namespace platform;
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

  LocalCountryFile localFile = LocalCountryFile::MakeForTesting(kCountryName);

  FrozenDataSource dataSource;
  auto result = dataSource.RegisterMap(localFile);
  TEST_EQUAL(result.second, MwmSet::RegResult::Success, ());

  auto const & id = result.first;
  MwmSet::MwmHandle handle = dataSource.GetMwmHandleById(id);
  TEST(handle.IsAlive(), ());

  auto const * value = handle.GetValue();
  FeaturesVector fv(value->m_cont, value->GetHeader(), value->m_ftTable.get(), value->m_relTable.get(),
                    value->m_metaDeserializer.get());

  map<string, int> actual;
  fv.ForEach([&](FeatureType & ft, uint32_t index)
  {
    string const postcode(ft.GetMetadata(feature::Metadata::FMD_POSTCODE));
    if (!postcode.empty())
      ++actual[postcode];
  });
  TEST_EQUAL(expected, actual, ());
}
}  // namespace features_vector_test
