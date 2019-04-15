#pragma once

#include "search/search_params.hpp"

#include "storage/downloader_search_params.hpp"

#include <functional>

class DataSource;

namespace storage
{
class CountryInfoGetter;
class Storage;
}  // namespace storage

namespace search
{
class Results;
struct SearchParamsBase;

// An on-results callback that should be used for the search in downloader.
//
// *NOTE* the class is NOT thread safe.
class DownloaderSearchCallback
{
public:
  class Delegate
  {
  public:
    virtual ~Delegate() = default;

    virtual void RunUITask(std::function<void()> fn) = 0;
  };

  DownloaderSearchCallback(Delegate & delegate, DataSource const & dataSource,
                           storage::CountryInfoGetter const & infoGetter,
                           storage::Storage const & storage,
                           storage::DownloaderSearchParams params);

  void operator()(Results const & results, SearchParamsBase const &);

private:
  Delegate & m_delegate;
  DataSource const & m_dataSource;
  storage::CountryInfoGetter const & m_infoGetter;
  storage::Storage const & m_storage;
  storage::DownloaderSearchParams m_params;
};
}  // namespace search
