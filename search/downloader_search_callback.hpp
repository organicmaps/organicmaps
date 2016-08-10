#pragma once

#include "storage/downloader_search_params.hpp"

#include "std/set.hpp"
#include "std/string.hpp"

class Index;

namespace storage
{
class CountryInfoGetter;
}  // namespace storage

// todo(@m)
// add tests

namespace search
{
class Results;

// An on-results callback that should be used for the search in downloader.
//
// *NOTE* the class is NOT thread safe.
class DownloaderSearchCallback
{
public:
  using TOnResults = storage::DownloaderSearchParams::TOnResults;

  DownloaderSearchCallback(Index const & index, storage::CountryInfoGetter const & infoGetter,
                           storage::DownloaderSearchParams params);

  void operator()(search::Results const & results);

private:
  set<storage::DownloaderSearchResult> m_uniqueResults;

  Index const & m_index;
  storage::CountryInfoGetter const & m_infoGetter;
  storage::DownloaderSearchParams m_params;
};
}  // namespace search
