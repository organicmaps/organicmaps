#include "testing/testing.hpp"

#include "map/framework.hpp"

#include "search/categories_cache.hpp"
#include "search/downloader_search_callback.hpp"
#include "search/mwm_context.hpp"
#include "search/utils.hpp"

#include "storage/storage.hpp"

#include "indexer/data_source.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/mwm_set.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "base/cancellable.hpp"
#include "base/checked_cast.hpp"

#include <algorithm>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace platform;
using namespace storage;
using namespace std;

UNIT_TEST(CountriesNamesTest)
{
  Framework f(FrameworkParams(false /* m_enableLocalAds */, false /* m_enableDiffs */));
  auto & storage = f.GetStorage();

  auto handle = search::FindWorld(f.GetDataSource());
  TEST(handle.IsAlive(), ());

  FeaturesLoaderGuard g(f.GetDataSource(), handle.GetId());

  auto & value = *handle.GetValue<MwmValue>();
  TEST(value.HasSearchIndex(), ());
  search::MwmContext const mwmContext(move(handle));
  base::Cancellable const cancellable;
  search::CategoriesCache cache(ftypes::IsLocalityChecker::Instance(), cancellable);

  vector<int8_t> const langIndices = {StringUtf8Multilang::kEnglishCode,
                                      StringUtf8Multilang::kDefaultCode,
                                      StringUtf8Multilang::kInternationalCode};

  // todo: (@t.yan) fix names for countries which have separate mwms.
  set<string> const kIgnoreList = {"American Samoa",
                                   "Sāmoa",
                                   "Pitcairn",
                                   "South Georgia and South Sandwich Islands",
                                   "Lesotho",
                                   "Eswatini",
                                   "Republic of the Congo",
                                   "Democratic Republic of the Congo",
                                   "Aruba",
                                   "Sint Maarten",
                                   "Bahamas",
                                   "Cabo Verde",
                                   "Ivory Coast",
                                   "Palestinian Territories",
                                   "Turkish Republic Of Northern Cyprus",
                                   "Nagorno-Karabakh Republic",
                                   "Vatican City",
                                   "North Macedonia",
                                   "Kosovo",
                                   "Czechia",
                                   "Transnistria",
                                   "Republic of Belarus",
                                   "Hong Kong",
                                   "Guam",
                                   // MAPSME-10611
                                   "Mayorca Residencial",
                                   "Magnolias Residencial"};

  auto const features = cache.Get(mwmContext);
  features.ForEach([&](uint64_t fid) {
    auto ft = g.GetFeatureByIndex(base::asserted_cast<uint32_t>(fid));
    TEST(ft, ());

    ftypes::Type const type = ftypes::IsLocalityChecker::Instance().GetType(*ft);
    if (type != ftypes::COUNTRY)
      return;

    TEST(any_of(langIndices.begin(), langIndices.end(),
                [&](uint8_t langIndex) {
                  string name;
                  if (!ft->GetName(langIndex, name))
                    return false;
                  return storage.IsNode(name) || kIgnoreList.count(name) != 0;
                }),
         ("Cannot find countries.txt record for country feature:",
          ft->DebugString(FeatureType::BEST_GEOMETRY)));
  });
}
