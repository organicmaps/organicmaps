#include "search/downloader_search_callback.hpp"

#include "search/result.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"

#include "indexer/index.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <set>
#include <string>

namespace
{
bool GetGroupCountryIdFromFeature(storage::Storage const & storage, FeatureType const & ft,
                                  std::string & name)
{
  int8_t const langIndices[] = {StringUtf8Multilang::kEnglishCode,
                                StringUtf8Multilang::kDefaultCode,
                                StringUtf8Multilang::kInternationalCode};

  for (auto const langIndex : langIndices)
  {
    if (!ft.GetName(langIndex, name))
      continue;
    if (storage.IsInnerNode(name))
      return true;
  }
  return false;
}
}  // namespace

namespace search
{
DownloaderSearchCallback::DownloaderSearchCallback(Delegate & delegate, Index const & index,
                                                   storage::CountryInfoGetter const & infoGetter,
                                                   storage::Storage const & storage,
                                                   storage::DownloaderSearchParams params)
  : m_delegate(delegate)
  , m_index(index)
  , m_infoGetter(infoGetter)
  , m_storage(storage)
  , m_params(move(params))
{
}

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
      Index::FeaturesLoaderGuard loader(m_index, fid.m_mwmId);
      FeatureType ft;
      if (!loader.GetFeatureByIndex(fid.m_index, ft))
      {
        LOG(LERROR, ("Feature can't be loaded:", fid));
        continue;
      }

      ftypes::Type const type = ftypes::IsLocalityChecker::Instance().GetType(ft);

      if (type == ftypes::COUNTRY || type == ftypes::STATE)
      {
        std::string groupFeatureName;
        if (GetGroupCountryIdFromFeature(m_storage, ft, groupFeatureName))
        {
          storage::DownloaderSearchResult downloaderResult(groupFeatureName,
                                                           result.GetString() /* m_matchedName */);
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
    storage::TCountryId const & countryId = m_infoGetter.GetRegionCountryId(mercator);
    if (countryId == storage::kInvalidCountryId)
      continue;

    storage::DownloaderSearchResult downloaderResult(countryId,
                                                     result.GetString() /* m_matchedName */);
    if (uniqueResults.find(downloaderResult) == uniqueResults.end())
    {
      uniqueResults.insert(downloaderResult);
      downloaderSearchResults.m_results.push_back(downloaderResult);
    }
  }

  downloaderSearchResults.m_query = m_params.m_query;
  downloaderSearchResults.m_endMarker = results.IsEndMarker();

  if (m_params.m_onResults)
  {
    auto onResults = m_params.m_onResults;
    m_delegate.RunUITask(
        [onResults, downloaderSearchResults]() { onResults(downloaderSearchResults); });
  }
}
}  // namespace search
