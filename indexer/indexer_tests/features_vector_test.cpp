#include "testing/testing.hpp"

#include "indexer/data_source.hpp"
#include "indexer/features_vector.hpp"
#include "indexer/mwm_set.hpp"

#include "platform/local_country_file.hpp"

#include <map>
#include <string>
#include <vector>

using namespace platform;
using namespace std;

namespace
{
// Postcodes with frequences.
// One can easily get this list of frequences with postcodes:
//
// cat data/minsk-pass.osm.bz2 | bunzip2 | grep 'addr:postcode' |
// sed "s/.*v='\(.*\)'.*/\1/g" |
// sort | uniq -c | awk '{ printf("{%s, %s},\n", $2, $1) }'
//
// But note, as one relation with id 3817793 and postcode 220039 is
// broken and is not included in minsk-pass.mwm, corresponding
// postcode frequency is decremented from 32 to 31.
vector<pair<int, int>> kCodeFreq = {{220000, 1},
                                    {220001, 1},
                                    {220004, 9},
                                    {220006, 131},
                                    {220007, 212},
                                    {220010, 4},
                                    {220011, 1},
                                    {220030, 183},
                                    {220036, 72},
                                    {220039, 31},
                                    {220050, 1},
                                    {721806, 1},
                                    {721816, 1},
                                    {721819, 1},
                                    {721882, 1}};

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

  auto const * value = handle.GetValue<MwmValue>();
  FeaturesVector fv(value->m_cont, value->GetHeader(), value->m_table.get());

  map<string, int> actual;
  fv.ForEach([&](FeatureType & ft, uint32_t index)
             {
               string postcode = ft.GetMetadata().Get(feature::Metadata::FMD_POSTCODE);
               if (!postcode.empty())
                 ++actual[postcode];
             });
  TEST_EQUAL(expected, actual, ());
}
}  // namespace
