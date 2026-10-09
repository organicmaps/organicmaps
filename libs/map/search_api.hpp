#pragma once

#include "map/bookmark_helpers.hpp"
#include "map/everywhere_search_params.hpp"
#include "map/viewport_search_callback.hpp"
#include "map/viewport_search_params.hpp"

#include "search/downloader_search_callback.hpp"
#include "search/engine.hpp"
#include "search/mode.hpp"
#include "search/query_saver.hpp"
#include "search/result.hpp"
#include "search/search_params.hpp"

#include "geometry/any_rect2d.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/timer.hpp"

#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

class DataSource;

namespace search
{
struct BookmarksSearchParams;
}  // namespace search

namespace storage
{
class CountryInfoGetter;
class Storage;
struct DownloaderSearchParams;
}  // namespace storage

class SearchAPI
  : public search::DownloaderSearchCallback::Delegate
  , public search::ViewportSearchCallback::Delegate
{
public:
  struct Delegate
  {
    virtual ~Delegate() = default;

    virtual void RunUITask(std::function<void()> /* fn */) {}

    using ResultsIterT = search::Results::ConstIter;
    virtual void ShowViewportSearchResults(ResultsIterT begin, ResultsIterT end, bool clear) {}

    virtual void ClearViewportSearchResults() {}

    virtual std::optional<m2::PointD> GetCurrentPosition() const { return {}; }

    virtual bool ParseSearchQueryCommand(search::SearchParams const & /* params */) { return false; }

    virtual m2::PointD GetMinDistanceBetweenResults() const { return {0, 0}; }
  };

  SearchAPI(DataSource & dataSource, storage::Storage const & storage, storage::CountryInfoGetter const & infoGetter,
            size_t numThreads, Delegate & delegate);
  virtual ~SearchAPI() = default;

  void OnViewportChanged(m2::RectD const & viewport, int scale = 0);

  struct AddressQuery
  {
    std::string m_query;
    // Normalized house-and-street portion, excluding geographical context.
    std::string m_expectedStreet;
  };
  using AddressCallback = std::function<void(std::optional<search::Result>)>;
  // UI-thread operation, independent of interactive results and search history.
  void ResolveAddress(uint64_t id, std::vector<AddressQuery> queries, std::string locale, bool background,
                      AddressCallback callback);
  void CancelAddressResolution(uint64_t id);
  void SetAddressViewportCallback(std::function<void(m2::RectD const &, int)> callback);

  void InitAfterWorldLoaded() { m_engine.InitAfterWorldLoaded(); }

  // Search everywhere.
  bool SearchEverywhere(search::EverywhereSearchParams params);

  // Search in the viewport.
  bool SearchInViewport(search::ViewportSearchParams params);

  // Search for maps by countries or cities.
  bool SearchInDownloader(storage::DownloaderSearchParams params);

  // Search for bookmarks.
  bool SearchInBookmarks(search::BookmarksSearchParams params);

  search::Engine & GetEngine() { return m_engine; }

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
  bool IsViewportSearchActive() const override;
  void ShowViewportSearchResults(search::Results::ConstIter begin, search::Results::ConstIter end, bool clear) override;

  std::list<search::QuerySaver::SearchRequest> const & GetLastSearchQueries() const { return m_searchQuerySaver.Get(); }
  void SaveSearchQuery(search::QuerySaver::SearchRequest const & query) { m_searchQuerySaver.Add(query); }
  void ClearSearchHistory() { m_searchQuerySaver.Clear(); }

  void EnableIndexingOfBookmarksDescriptions(bool enable);

  void SetLocale(std::string const & locale);

  // By default all created bookmarks are saved in BookmarksProcessor
  // but we do not index them in an attempt to save time and memory.
  // This method must be used to enable or disable indexing all current and future
  // bookmarks belonging to |groupId|.
  void EnableIndexingOfBookmarkGroup(kml::MarkGroupId const & groupId, bool enable);

  // Returns the bookmarks search to its default, pre-launch state.
  // This includes dropping all bookmark data for created bookmarks (efficiently
  // calling OnBookmarksDeleted with all known bookmarks as an argument),
  // clearing the bookmark search index, and resetting all parameters to
  // their default values.
  void ResetBookmarksEngine();

  void OnBookmarksCreated(std::vector<BookmarkInfo> const & marks);
  void OnBookmarksUpdated(std::vector<BookmarkInfo> const & marks);
  void OnBookmarksDeleted(std::vector<kml::MarkId> const & marks);
  void OnBookmarksAttached(std::vector<BookmarkGroupInfo> const & groupInfos);
  void OnBookmarksDetached(std::vector<BookmarkGroupInfo> const & groupInfos);

private:
  struct SearchIntent
  {
    search::SearchParams m_params;
    std::weak_ptr<search::ProcessorHandle> m_handle;
    bool m_isDelayed = false;
    bool m_isRunning = false;
    uint64_t m_generation = 0;
  };

  struct AddressRequest
  {
    uint64_t m_id;
    std::vector<AddressQuery> m_queries;
    std::string m_locale;
    bool m_background;
    AddressCallback m_callback;
    size_t m_queryIndex = 0;
    base::Timer m_timer;
  };
  void StartAddressResolution();
  void SuspendAddressResolution();

  bool Search(search::SearchParams params, bool forceSearch);
  void Search(SearchIntent & intent);

  void SetViewportIfPossible(search::SearchParams & params);

  bool QueryMayBeSkipped(search::SearchParams const & prevParams, search::SearchParams const & currParams) const;

  DataSource & m_dataSource;
  storage::Storage const & m_storage;
  storage::CountryInfoGetter const & m_infoGetter;
  Delegate & m_delegate;

  search::Engine m_engine;

  search::QuerySaver m_searchQuerySaver;

  // Descriptions of last search queries for different modes. May be
  // used for search requests skipping. This field is not guarded
  // because it must be used from the UI thread only.
  SearchIntent m_searchIntents[static_cast<size_t>(search::Mode::Count)];
  std::deque<std::shared_ptr<AddressRequest>> m_addressRequests;
  std::weak_ptr<search::ProcessorHandle> m_addressHandle;
  bool m_addressRunning = false;
  uint64_t m_addressGeneration = 0;
  std::function<void(m2::RectD const &, int)> m_addressViewportCallback;
  int m_viewportScale = 0;

  m2::RectD m_viewport;
  bool m_isViewportInitialized = false;

  // Viewport search callback should be changed every time when SearchAPI::PokeSearchInViewport
  // is called and we need viewport search params to construct it.
  search::ViewportSearchParams m_viewportParams;

  // Same as the one in bookmarks::Processor. Duplicated here because
  // it is easier than obtaining the information about a group asynchronously
  // from |m_engine|.
  std::unordered_set<kml::MarkGroupId> m_indexableGroups;
};

namespace search
{
// Adjusts |viewport| to show the search results, keeping its rotation. Only the best matching results
// (with the minimum number of misprints) are taken into account:
// - one of them is already visible: nothing changes;
// - the nearest one is not far away: zooms out around the center (keeping the extents) to include it;
// - all of them are localized: shows all of them;
// - otherwise shows the top ranked one at its own scale.
// Returns true if |viewport| was changed.
bool AdjustViewportToSearchResults(Results const & results, m2::AnyRectD & viewport);
}  // namespace search
