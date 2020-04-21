#include "testing/testing.hpp"

#include "map/framework.hpp"

#include "search/categories_cache.hpp"
#include "search/downloader_search_callback.hpp"
#include "search/mwm_context.hpp"

#include "storage/storage.hpp"

#include "indexer/data_source.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/utils.hpp"

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
  auto const & synonyms = storage.GetCountryNameSynonyms();

  auto handle = indexer::FindWorld(f.GetDataSource());
  TEST(handle.IsAlive(), ());

  FeaturesLoaderGuard g(f.GetDataSource(), handle.GetId());

  auto & value = *handle.GetValue();
  TEST(value.HasSearchIndex(), ());
  search::MwmContext const mwmContext(move(handle));
  base::Cancellable const cancellable;
  search::CategoriesCache cache(ftypes::IsLocalityChecker::Instance(), cancellable);

  vector<int8_t> const langIndices = {StringUtf8Multilang::kEnglishCode,
                                      StringUtf8Multilang::kDefaultCode,
                                      StringUtf8Multilang::kInternationalCode};

  set<string> const kIgnoreList = {"Turkish Republic Of Northern Cyprus",
                                   "Transnistria",
                                   "Nagorno-Karabakh Republic",
                                   "Republic of Artsakh",
                                   };

  auto const features = cache.Get(mwmContext);
  features.ForEach([&](uint64_t fid) {
    auto ft = g.GetFeatureByIndex(base::asserted_cast<uint32_t>(fid));
    TEST(ft, ());

    ftypes::LocalityType const type = ftypes::IsLocalityChecker::Instance().GetType(*ft);
    if (type != ftypes::LocalityType::Country)
      return;

    TEST(any_of(langIndices.begin(), langIndices.end(),
                [&](uint8_t langIndex) {
                  string name;
                  if (!ft->GetName(langIndex, name))
                    return false;
                  auto const it = synonyms.find(name);
                  if (it == synonyms.end())
                    return storage.IsNode(name) || kIgnoreList.count(name) != 0;
                  return storage.IsNode(it->second);
                }),
         ("Cannot find countries.txt record for country feature:",
          ft->DebugString(FeatureType::BEST_GEOMETRY)));
  });
}
