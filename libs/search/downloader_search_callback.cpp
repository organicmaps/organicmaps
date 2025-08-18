#include "search/downloader_search_callback.hpp"

#include "search/result.hpp"

#include "editor/editable_data_source.hpp"

#include "indexer/data_source.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <set>
#include <string>
#include <utility>

namespace
{
/// @todo Can't change on string_view now, because of unordered_map<string> synonyms.
bool GetGroupCountryIdFromFeature(storage::Storage const & storage, FeatureType & ft, std::string & name)
{
  auto const & synonyms = storage.GetCountryNameSynonyms();
  int8_t const langIndices[] = {StringUtf8Multilang::kEnglishCode, StringUtf8Multilang::kDefaultCode,
                                StringUtf8Multilang::kInternationalCode};

  for (auto const langIndex : langIndices)
  {
    name = ft.GetName(langIndex);
    if (name.empty())
      continue;

    if (storage.IsInnerNode(name))
      return true;
    auto const it = synonyms.find(name);
    if (it == synonyms.end())
      continue;
    if (!storage.IsInnerNode(it->second))
      continue;
    name = it->second;
    return true;
  }
  return false;
}
}  // namespace

namespace search
{
DownloaderSearchCallback::DownloaderSearchCallback(Delegate & delegate, DataSource const & dataSource,
                                                   storage::CountryInfoGetter const & infoGetter,
                                                   storage::Storage const & storage,
                                                   storage::DownloaderSearchParams params)
  : m_delegate(delegate)
  , m_dataSource(dataSource)
  , m_infoGetter(infoGetter)
  , m_storage(storage)
  , m_params(std::move(params))
{}

void DownloaderSearchCallback::operator()(search::Results const & results)
{
  storage::DownloaderSearchResults downloaderSearchResults;
  std::set<storage::DownloaderSearchResult> uniqueResults;

  for (auto const & result : results)
  {
    if (!result.HasPoint())
      continue;

    if (result.GetResultType() != search::Result::Type::LatLon)
    {
      FeatureID const & fid = result.GetFeatureID();
      FeaturesLoaderGuard loader(m_dataSource, fid.m_mwmId);
      auto ft = loader.GetFeatureByIndex(fid.m_index);
      if (!ft)
      {
        LOG(LERROR, ("Feature can't be loaded:", fid));
        continue;
      }

      ftypes::LocalityType const type = ftypes::IsLocalityChecker::Instance().GetType(*ft);

      if (type == ftypes::LocalityType::Country || type == ftypes::LocalityType::State)
      {
        std::string groupFeatureName;
        if (GetGroupCountryIdFromFeature(m_storage, *ft, groupFeatureName))
        {
          storage::DownloaderSearchResult downloaderResult(groupFeatureName, result.GetString() /* m_matchedName */);
          if (uniqueResults.find(downloaderResult) == uniqueResults.end())
          {
            uniqueResults.insert(downloaderResult);
            downloaderSearchResults.m_results.push_back(downloaderResult);
          }
          continue;
        }
      }
    }
    auto const & mercator = result.GetFeatureCenter();
    storage::CountryId const & countryId = m_infoGetter.GetRegionCountryId(mercator);
    if (countryId == storage::kInvalidCountryId)
      continue;

    storage::DownloaderSearchResult downloaderResult(countryId, result.GetString() /* m_matchedName */);
    if (uniqueResults.find(downloaderResult) == uniqueResults.end())
    {
      uniqueResults.insert(downloaderResult);
      downloaderSearchResults.m_results.push_back(downloaderResult);
    }
  }

  downloaderSearchResults.m_query = m_params.m_query;
  downloaderSearchResults.m_endMarker = results.IsEndMarker();

  m_delegate.RunUITask([onResults = m_params.m_onResults, results = std::move(downloaderSearchResults)]() mutable
  { onResults(std::move(results)); });
}
}  // namespace search
