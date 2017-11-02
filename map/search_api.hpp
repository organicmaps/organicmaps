#pragma once

#include "search/downloader_search_callback.hpp"
#include "search/engine.hpp"
#include "search/everywhere_search_callback.hpp"
#include "search/mode.hpp"
#include "search/result.hpp"
#include "search/search_params.hpp"
#include "search/viewport_search_callback.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>

#include <boost/optional.hpp>

namespace search
{
struct EverywhereSearchParams;
struct ViewportSearchParams;
}

namespace storage
{
class CountryInfoGetter;
class Storage;
struct DownloaderSearchParams;
}

class SearchAPI : public search::DownloaderSearchCallback::Delegate,
                  public search::EverywhereSearchCallback::Delegate,
                  public search::ViewportSearchCallback::Delegate
{
public:
  struct Delegate
  {
    virtual ~Delegate() = default;

    virtual void RunUITask(std::function<void()> /* fn */) {}
    virtual void SetSearchDisplacementModeEnabled(bool /* enabled */) {}
    virtual void ShowViewportSearchResults(search::Results const & /* results */) {}
    virtual void ClearViewportSearchResults() {}

    virtual boost::optional<m2::PointD> GetCurrentPosition() const { return {}; };
    virtual bool ParseMagicSearchQuery(search::SearchParams const & /* params */) { return false; };
    virtual double GetMinDistanceBetweenResults() const { return 0.0; };
    virtual bool IsLocalAdsCustomer(search::Result const & /* result */) const { return false; }
  };

  SearchAPI(Index & index, storage::Storage const & storage,
            storage::CountryInfoGetter const & infoGetter, Delegate & delegate);
  virtual ~SearchAPI() = default;

  void OnViewportChanged(m2::RectD const & viewport);

  void LoadCitiesBoundaries() { m_engine.LoadCitiesBoundaries(); }

  bool IsCianSearchMode() const { return m_cianSearchMode; }

  // Search everywhere.
  bool SearchEverywhere(search::EverywhereSearchParams const & params);

  // Search in the viewport.
  bool SearchInViewport(search::ViewportSearchParams const & params);

  // Search for maps by countries or cities.
  bool SearchInDownloader(storage::DownloaderSearchParams const & params);

  search::Engine & GetEngine() { return m_engine; }
  search::Engine const & GetEngine() const { return m_engine; }

  // When search in viewport is active or delayed, restarts search in
  // viewport. When |forceSearch| is false, request is skipped when it
  // is similar to the previous request in the current
  // search-in-viewport session.
  void PokeSearchInViewport(bool forceSearch = true);

  void CancelSearch(search::Mode mode);
  void CancelAllSearches();
  void ClearCaches() { return m_engine.ClearCaches(); }

  // *SearchCallback::Delegate overrides:
  void RunUITask(std::function<void()> fn) override;
  void SetHotelDisplacementMode() override;
  bool IsViewportSearchActive() const override;
  void ShowViewportSearchResults(search::Results const & results) override;
  void ClearViewportSearchResults() override;
  bool IsLocalAdsCustomer(search::Result const & result) const override;

private:
  struct SearchIntent
  {
    search::SearchParams m_params;
    std::weak_ptr<search::ProcessorHandle> m_handle;
    bool m_isDelayed = false;
  };

  bool Search(search::SearchParams const & params, bool forceSearch);
  void Search(SearchIntent & intent);

  void SetViewportIfPossible(search::SearchParams & params);

  bool QueryMayBeSkipped(search::SearchParams const & prevParams,
                         search::SearchParams const & currParams) const;

  Index & m_index;
  storage::Storage const & m_storage;
  storage::CountryInfoGetter const & m_infoGetter;
  Delegate & m_delegate;

  search::Engine m_engine;

  // Descriptions of last search queries for different modes. May be
  // used for search requests skipping. This field is not guarded
  // because it must be used from the UI thread only.
  SearchIntent m_searchIntents[static_cast<size_t>(search::Mode::Count)];

  m2::RectD m_viewport;
  bool m_isViewportInitialized = false;

  // TODO (@y, @m): delete me after Cian project is finished.
  bool m_cianSearchMode = false;
};
