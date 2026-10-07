#include "map/search_api.hpp"
#include "search/address_matcher.hpp"

#include "map/bookmarks_search_params.hpp"
#include "map/everywhere_search_params.hpp"

#include "search/geometry_utils.hpp"
#include "search/utils.hpp"

#include "storage/downloader_search_params.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/scales.hpp"

#include "platform/preferred_languages.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <string>
#include <type_traits>

using namespace search;
namespace
{
using BookmarkIdDoc = std::pair<bookmarks::Id, bookmarks::Doc>;

double const kDistEqualQueryMeters = 100.0;
double const kDistEqualQueryMercator = mercator::MetersToMercator(kDistEqualQueryMeters);

// Cancels search query by |handle|.
void CancelQuery(std::weak_ptr<ProcessorHandle> & handle)
{
  if (auto queryHandle = handle.lock())
    queryHandle->Cancel();
  handle.reset();
}

bookmarks::Id KmlMarkIdToSearchBookmarkId(kml::MarkId id)
{
  static_assert(std::is_integral<kml::MarkId>::value, "");
  static_assert(std::is_integral<bookmarks::Id>::value, "");

  static_assert(std::is_unsigned<kml::MarkId>::value, "");
  static_assert(std::is_unsigned<bookmarks::Id>::value, "");

  static_assert(sizeof(bookmarks::Id) == sizeof(kml::MarkId), "");

  return base::asserted_cast<bookmarks::Id>(id);
}

bookmarks::GroupId KmlGroupIdToSearchGroupId(kml::MarkGroupId id)
{
  static_assert(std::is_integral<kml::MarkGroupId>::value, "");
  static_assert(std::is_integral<bookmarks::GroupId>::value, "");

  static_assert(std::is_unsigned<kml::MarkGroupId>::value, "");
  static_assert(std::is_unsigned<bookmarks::GroupId>::value, "");

  static_assert(sizeof(bookmarks::GroupId) >= sizeof(kml::MarkGroupId), "");

  if (id == kml::kInvalidMarkGroupId)
    return bookmarks::kInvalidGroupId;

  return base::asserted_cast<bookmarks::GroupId>(id);
}

kml::MarkId SearchBookmarkIdToKmlMarkId(bookmarks::Id id)
{
  return static_cast<kml::MarkId>(id);
}

void AppendBookmarkIdDocs(std::vector<BookmarkInfo> const & marks, std::vector<BookmarkIdDoc> & result)
{
  result.reserve(result.size() + marks.size());

  auto const locale = languages::GetCurrentOrig();
  for (auto const & mark : marks)
    result.emplace_back(KmlMarkIdToSearchBookmarkId(mark.m_bookmarkId), bookmarks::Doc(*mark.m_bookmarkData, locale));
}

void AppendBookmarkIds(std::vector<kml::MarkId> const & marks, std::vector<bookmarks::Id> & result)
{
  result.reserve(result.size() + marks.size());
  std::transform(marks.begin(), marks.end(), std::back_inserter(result), KmlMarkIdToSearchBookmarkId);
}

class BookmarksSearchCallback
{
public:
  using OnResults = BookmarksSearchParams::OnResults;

  BookmarksSearchCallback(SearchAPI::Delegate & delegate, OnResults onResults)
    : m_delegate(delegate)
    , m_onResults(std::move(onResults))
  {}

  void operator()(Results const & results)
  {
    if (results.IsEndMarker())
    {
      if (results.IsEndedNormal())
      {
        m_status = BookmarksSearchParams::Status::Completed;
      }
      else
      {
        ASSERT(results.IsEndedCancelled(), ());
        m_status = BookmarksSearchParams::Status::Cancelled;
      }
    }
    else
    {
      ASSERT_EQUAL(m_status, BookmarksSearchParams::Status::InProgress, ());
    }

    auto const & rs = results.GetBookmarksResults();
    ASSERT_LESS_OR_EQUAL(m_results.size(), rs.size(), ());

    for (size_t i = m_results.size(); i < rs.size(); ++i)
      m_results.emplace_back(SearchBookmarkIdToKmlMarkId(rs[i].m_id));

    m_delegate.RunUITask([onResults = m_onResults, results = m_results, status = m_status]() mutable
    { onResults(std::move(results), status); });
  }

private:
  BookmarksSearchParams::Results m_results;
  BookmarksSearchParams::Status m_status = BookmarksSearchParams::Status::InProgress;

  SearchAPI::Delegate & m_delegate;
  OnResults m_onResults;
};
}  // namespace

SearchAPI::SearchAPI(DataSource & dataSource, storage::Storage const & storage,
                     storage::CountryInfoGetter const & infoGetter, size_t numThreads, Delegate & delegate)
  : m_dataSource(dataSource)
  , m_storage(storage)
  , m_infoGetter(infoGetter)
  , m_delegate(delegate)
  , m_engine(m_dataSource, GetDefaultCategories(), m_infoGetter,
             Engine::Params(languages::GetCurrentMapTwine() /* locale */, numThreads))
{}

void SearchAPI::OnViewportChanged(m2::RectD const & viewport, int scale)
{
  // The map may be scrolled past the antimeridian, while the search engine works with canonical coordinates only.
  /// @todo A rect crossing the antimeridian is searched in its canonical part only: the engine (mwm selection,
  /// features collection, pivot distances) doesn't support two-parts rects.
  m_viewport = mercator::WrapRectX(viewport);
  m_viewportScale = scale;

  auto const forceSearchInViewport = !m_isViewportInitialized;
  if (!m_isViewportInitialized)
  {
    m_isViewportInitialized = true;
    for (size_t i = 0; i < static_cast<size_t>(Mode::Count); i++)
    {
      auto & intent = m_searchIntents[i];
      // Viewport search will be triggered below, in PokeSearchInViewport().
      if (!intent.m_isDelayed || static_cast<Mode>(i) == Mode::Viewport)
        continue;
      intent.m_params.m_viewport = m_viewport;
      intent.m_params.m_position = m_delegate.GetCurrentPosition();
      Search(intent);
    }
  }

  PokeSearchInViewport(forceSearchInViewport);
  if (m_addressViewportCallback)
    m_addressViewportCallback(m_viewport, scale);
  StartAddressResolution();
}

bool SearchAPI::SearchEverywhere(EverywhereSearchParams params)
{
  CHECK(params.m_onResults, ());

  SearchParams p;
  p.m_query = std::move(params.m_query);
  p.m_inputLocale = std::move(params.m_inputLocale);
  p.m_mode = Mode::Everywhere;
  p.m_position = m_delegate.GetCurrentPosition();
  SetViewportIfPossible(p);  // Search request will be delayed if viewport is not available.
  p.m_maxNumResults = SearchParams::kDefaultNumResultsEverywhere;
  p.m_suggestsEnabled = true;
  p.m_needAddress = true;
  p.m_needHighlighting = true;
  p.m_categorialRequest = params.m_isCategory;
  bool const prioritizeAddressMatches =
      params.m_prioritizeAddressMatches && !params.m_isCategory && search::IsAddressQuery(p.m_query);
  p.m_allowNearbyHouseNumbers = prioritizeAddressMatches;
  if (params.m_timeout)
    p.m_timeout = *params.m_timeout;

  p.m_onResults = [this, onResults = std::move(params.m_onResults), query = p.m_query,
                   prioritizeAddressMatches](Results const & results)
  {
    auto resolved = prioritizeAddressMatches ? search::RankAddressResults(query, results) : results;
    RunUITask([onResults, results = std::move(resolved)]() mutable { onResults(std::move(results)); });
  };

  return Search(std::move(p), true /* forceSearch */);
}

bool SearchAPI::SearchInViewport(ViewportSearchParams params)
{
  // Save params first for the PokeSearchInViewport function.
  m_viewportParams = params;

  SearchParams p;
  p.m_query = std::move(params.m_query);
  p.m_inputLocale = std::move(params.m_inputLocale);
  p.m_position = m_delegate.GetCurrentPosition();
  SetViewportIfPossible(p);  // Search request will be delayed if viewport is not available.
  p.m_maxNumResults = SearchParams::kDefaultNumResultsInViewport;
  p.m_mode = Mode::Viewport;
  p.m_suggestsEnabled = false;
  p.m_needAddress = false;
  p.m_needHighlighting = false;
  p.m_categorialRequest = params.m_isCategory;

  if (params.m_timeout)
    p.m_timeout = *params.m_timeout;

  if (params.m_onStarted)
  {
    p.m_onStarted = [this, onStarted = std::move(params.m_onStarted)]() mutable
    { RunUITask([onStarted = std::move(onStarted)]() { onStarted(); }); };
  }

  p.m_onResults = ViewportSearchCallback(m_viewport, *this, std::move(params.m_onCompleted));

  return Search(std::move(p), false /* forceSearch */);
}

bool SearchAPI::SearchInDownloader(storage::DownloaderSearchParams params)
{
  SearchParams p;
  p.m_query = params.m_query;
  p.m_inputLocale = params.m_inputLocale;
  p.m_position = m_delegate.GetCurrentPosition();
  SetViewportIfPossible(p);  // Search request will be delayed if viewport is not available.
  p.m_maxNumResults = SearchParams::kDefaultNumResultsEverywhere;
  p.m_mode = Mode::Downloader;
  p.m_suggestsEnabled = false;
  p.m_needAddress = false;
  p.m_needHighlighting = false;

  p.m_onResults = DownloaderSearchCallback(*this, m_dataSource, m_infoGetter, m_storage, std::move(params));

  return Search(std::move(p), true /* forceSearch */);
}

bool SearchAPI::SearchInBookmarks(search::BookmarksSearchParams params)
{
  SearchParams p;
  p.m_query = std::move(params.m_query);
  p.m_position = m_delegate.GetCurrentPosition();
  SetViewportIfPossible(p);  // Search request will be delayed if viewport is not available.
  p.m_maxNumResults = SearchParams::kDefaultNumBookmarksResults;
  p.m_mode = Mode::Bookmarks;
  p.m_suggestsEnabled = false;
  p.m_needAddress = false;

  p.m_bookmarksGroupId = params.m_groupId;
  p.m_onResults = BookmarksSearchCallback(m_delegate, std::move(params.m_onResults));

  return Search(std::move(p), true /* forceSearch */);
}

void SearchAPI::PokeSearchInViewport(bool forceSearch)
{
  if (!m_isViewportInitialized || !IsViewportSearchActive())
    return;

  // Copy is intentional here, to skip possible duplicating requests.
  auto params = m_searchIntents[static_cast<size_t>(Mode::Viewport)].m_params;
  SetViewportIfPossible(params);
  params.m_position = m_delegate.GetCurrentPosition();
  params.m_onResults = ViewportSearchCallback(m_viewport, *this, m_viewportParams.m_onCompleted);

  Search(std::move(params), forceSearch);
}

void SearchAPI::CancelSearch(Mode mode)
{
  ASSERT_NOT_EQUAL(mode, Mode::Count, ());

  if (mode == Mode::Viewport)
    m_delegate.ClearViewportSearchResults();

  auto & intent = m_searchIntents[static_cast<size_t>(mode)];
  intent.m_params.Clear();
  CancelQuery(intent.m_handle);
  intent.m_isRunning = false;
  ++intent.m_generation;
  StartAddressResolution();
}

void SearchAPI::CancelAllSearches()
{
  for (size_t i = 0; i < static_cast<size_t>(Mode::Count); ++i)
    CancelSearch(static_cast<Mode>(i));
}

void SearchAPI::RunUITask(std::function<void()> fn)
{
  return m_delegate.RunUITask(std::move(fn));
}

bool SearchAPI::IsViewportSearchActive() const
{
  return !m_searchIntents[static_cast<size_t>(Mode::Viewport)].m_params.m_query.empty();
}

void SearchAPI::ShowViewportSearchResults(Results::ConstIter begin, Results::ConstIter end, bool clear)
{
  return m_delegate.ShowViewportSearchResults(begin, end, clear);
}

void SearchAPI::EnableIndexingOfBookmarksDescriptions(bool enable)
{
  m_engine.EnableIndexingOfBookmarksDescriptions(enable);
}

void SearchAPI::EnableIndexingOfBookmarkGroup(kml::MarkGroupId const & groupId, bool enable)
{
  if (enable)
    m_indexableGroups.insert(groupId);
  else
    m_indexableGroups.erase(groupId);

  m_engine.EnableIndexingOfBookmarkGroup(KmlGroupIdToSearchGroupId(groupId), enable);
}

void SearchAPI::ResetBookmarksEngine()
{
  m_indexableGroups.clear();
  m_engine.ResetBookmarks();
}

void SearchAPI::OnBookmarksCreated(std::vector<BookmarkInfo> const & marks)
{
  std::vector<BookmarkIdDoc> data;
  AppendBookmarkIdDocs(marks, data);
  m_engine.OnBookmarksCreated(data);
}

void SearchAPI::OnBookmarksUpdated(std::vector<BookmarkInfo> const & marks)
{
  std::vector<BookmarkIdDoc> data;
  AppendBookmarkIdDocs(marks, data);
  m_engine.OnBookmarksUpdated(data);
}

void SearchAPI::OnBookmarksDeleted(std::vector<kml::MarkId> const & marks)
{
  std::vector<bookmarks::Id> data;
  AppendBookmarkIds(marks, data);
  m_engine.OnBookmarksDeleted(data);
}

void SearchAPI::OnBookmarksAttached(std::vector<BookmarkGroupInfo> const & groupInfos)
{
  for (auto const & info : groupInfos)
  {
    std::vector<bookmarks::Id> data;
    AppendBookmarkIds(info.m_bookmarkIds, data);
    m_engine.OnBookmarksAttachedToGroup(KmlGroupIdToSearchGroupId(info.m_groupId), data);
  }
}

void SearchAPI::OnBookmarksDetached(std::vector<BookmarkGroupInfo> const & groupInfos)
{
  for (auto const & info : groupInfos)
  {
    std::vector<bookmarks::Id> data;
    AppendBookmarkIds(info.m_bookmarkIds, data);
    m_engine.OnBookmarksDetachedFromGroup(KmlGroupIdToSearchGroupId(info.m_groupId), data);
  }
}

bool SearchAPI::Search(SearchParams params, bool forceSearch)
{
  if (m_delegate.ParseSearchQueryCommand(params))
    return false;

  auto const mode = params.m_mode;
  auto & intent = m_searchIntents[static_cast<size_t>(mode)];

  if (!forceSearch && QueryMayBeSkipped(intent.m_params, params))
    return false;

  intent.m_params = std::move(params);

  // Cancels previous search request (if any) and initiates a new search request.
  CancelQuery(intent.m_handle);

  intent.m_params.m_minDistanceOnMapBetweenResults = m_delegate.GetMinDistanceBetweenResults();

  Search(intent);

  return true;
}

void SearchAPI::Search(SearchIntent & intent)
{
  SuspendAddressResolution();
  intent.m_isRunning = true;
  if (!m_isViewportInitialized)
  {
    intent.m_isDelayed = true;
    return;
  }

  auto params = intent.m_params;
  auto const generation = ++intent.m_generation;
  auto const mode = params.m_mode;
  params.m_onResults = [this, mode, generation, callback = params.m_onResults](Results const & results)
  {
    callback(results);
    if (results.IsEndMarker())
      RunUITask([this, mode, generation]
      {
        auto & current = m_searchIntents[static_cast<size_t>(mode)];
        if (current.m_generation != generation)
          return;
        current.m_isRunning = false;
        StartAddressResolution();
      });
  };
  intent.m_handle = m_engine.Search(std::move(params));
  intent.m_isDelayed = false;
}

void SearchAPI::SetAddressViewportCallback(std::function<void(m2::RectD const &, int)> callback)
{
  m_addressViewportCallback = std::move(callback);
  if (m_addressViewportCallback && m_isViewportInitialized)
    m_addressViewportCallback(m_viewport, m_viewportScale);
}

void SearchAPI::ResolveAddress(uint64_t id, std::vector<AddressQuery> queries, std::string locale, bool background,
                               AddressCallback callback)
{
  CHECK(callback, ());
  auto request =
      std::make_shared<AddressRequest>(id, std::move(queries), std::move(locale), background, std::move(callback));
  if (background)
    m_addressRequests.push_back(std::move(request));
  else
  {
    SuspendAddressResolution();
    m_addressRequests.push_front(std::move(request));
  }
  StartAddressResolution();
}

void SearchAPI::CancelAddressResolution(uint64_t id)
{
  if (!m_addressRequests.empty() && m_addressRequests.front()->m_id == id)
    SuspendAddressResolution();
  std::erase_if(m_addressRequests, [id](auto const & request) { return request->m_id == id; });
  StartAddressResolution();
}

void SearchAPI::SuspendAddressResolution()
{
  ++m_addressGeneration;
  CancelQuery(m_addressHandle);
  m_addressRunning = false;
}

void SearchAPI::StartAddressResolution()
{
  if (m_addressRunning || m_addressRequests.empty() || !m_isViewportInitialized)
    return;
  for (auto const & intent : m_searchIntents)
    if (intent.m_isRunning)
      return;

  auto const request = m_addressRequests.front();
  if (request->m_queries.empty())
  {
    m_addressRequests.pop_front();
    request->m_callback({});
    StartAddressResolution();
    return;
  }
  if (request->m_background && m_viewportScale < 16)
    return;

  m_addressRunning = true;
  auto const generation = ++m_addressGeneration;
  auto const query = request->m_queries[request->m_queryIndex];
  auto const viewport = m_viewport;
  SearchParams params;
  params.m_query = query.m_query + " ";
  params.m_inputLocale = request->m_locale;
  params.m_viewport = viewport;
  params.m_mode = request->m_background ? Mode::Viewport : Mode::Everywhere;
  params.m_needAddress = true;
  params.m_onResults = [this, request, generation, query, viewport](Results const & results)
  {
    std::optional<search::Result> earlyMatch;
    if (!results.IsEndMarker())
    {
      if (auto const * result = search::FindUniqueAddressResult(query.m_query, results, query.m_expectedStreet, true))
        if (!request->m_background || viewport.IsPointInside(result->GetFeatureCenter()))
          earlyMatch = *result;
      if (!earlyMatch)
        return;
    }
    std::optional<search::Result> match = earlyMatch;
    if (!match)
    {
      if (auto const * result = search::FindUniqueAddressResult(query.m_query, results, query.m_expectedStreet))
        if (!request->m_background || viewport.IsPointInside(result->GetFeatureCenter()))
          match = *result;
    }
    RunUITask([this, request, generation, match = std::move(match), early = earlyMatch.has_value()]() mutable
    {
      // Cancelled or suspended attempts must not consume a query or deliver a stale coordinate.
      if (generation != m_addressGeneration)
        return;
      m_addressRunning = false;
      // Deadlines retain useful results. Explicit cancellation/suspension is excluded by the generation guard.
      if (!match && ++request->m_queryIndex < request->m_queries.size())
      {
        StartAddressResolution();
        return;
      }
      SuspendAddressResolution();
      m_addressRequests.pop_front();
      LOG(LDEBUG, ("Address resolution", request->m_id, "background", request->m_background, "found", match.has_value(),
                   "early", early, "milliseconds", request->m_timer.ElapsedMilliseconds()));
      request->m_callback(std::move(match));
      StartAddressResolution();
    });
  };
  m_addressHandle = m_engine.Search(std::move(params));
}

void SearchAPI::SetViewportIfPossible(SearchParams & params)
{
  if (m_isViewportInitialized)
    params.m_viewport = m_viewport;
}

void SearchAPI::SetLocale(std::string const & locale)
{
  m_engine.SetLocale(locale);
}

bool SearchAPI::QueryMayBeSkipped(SearchParams const & prevParams, SearchParams const & currParams) const
{
  auto const & prevViewport = prevParams.m_viewport;
  auto const & currViewport = currParams.m_viewport;

  if (!prevParams.IsEqualCommon(currParams))
    return false;

  if (!prevViewport.IsValid() || !IsEqualMercator(prevViewport, currViewport, kDistEqualQueryMercator))
    return false;

  if (prevParams.m_position && currParams.m_position &&
      mercator::DistanceOnEarth(*prevParams.m_position, *currParams.m_position) > kDistEqualQueryMeters)
  {
    return false;
  }

  return true;
}

namespace search
{
namespace
{
// Results farther than this from the viewport center (or than the viewport's half diagonal multiplied by the factor)
// are shown by moving the viewport instead of zooming it out.
double constexpr kFarDistanceMeters = 20000.0;
double constexpr kFarViewportFactor = 2.0;
// Results that fit into a rect of this scale are considered localized and are shown all at once.
int constexpr kLocalizedScale = 15;
double constexpr kMarginFactor = 0.05;

// Mercator size of a square that is shown at |scale| (a tile of the world).
double GetSizeForScale(int scale)
{
  return mercator::Bounds::kRangeX / (1 << scale);
}

// The scale to show a single result at: its feature's own comfort scale (e.g. a city is shown farther than a cafe).
int GetScaleForResult(Result const & result)
{
  if (result.GetResultType() != Result::Type::Feature)
    return scales::GetUpperComfortScale();

  feature::TypesHolder types(feature::GeomType::Point);
  types.Assign(result.GetFeatureType());
  return feature::GetFeatureViewportScale(result.GetFeatureID(), types);
}

// Suggestions are query completions, not matches, although the ones made from features have a point.
bool IsMatchedResult(Result const & result)
{
  return result.HasPoint() && !result.IsSuggest();
}
}  // namespace

bool AdjustViewportToSearchResults(Results const & results, m2::AnyRectD & viewport)
{
  // Only the best matching results take part, e.g. a misprinted "Tribu Mins" should not outweigh "Minsk".
  uint16_t minErrors = ErrorsMade::kInfiniteErrors;
  for (auto const & r : results)
    if (IsMatchedResult(r))
      minErrors = std::min(minErrors, r.GetErrorsMade().m_errorsMade);

  m2::PointD const center = viewport.Center();
  // The viewport may be scrolled past the antimeridian: take the results in the world copy nearest to it.
  auto const nearestCopy = [&center](m2::PointD pt)
  {
    pt.x = mercator::NearestWrapX(pt.x, center.x);
    return pt;
  };

  Result const * top = nullptr;
  m2::PointD topPt, nearestPt;
  double minDistance = std::numeric_limits<double>::max();
  size_t count = 0;
  // Bounding rect of the results in the local coordinates of the viewport.
  m2::RectD bounds;
  for (auto const & r : results)
  {
    if (!IsMatchedResult(r) || r.GetErrorsMade().m_errorsMade != minErrors)
      continue;

    m2::PointD const pt = nearestCopy(r.GetFeatureCenter());
    if (viewport.IsPointInside(pt))
      return false;

    if (!top)
    {
      top = &r;
      topPt = pt;
    }
    ++count;
    bounds.Add(viewport.ConvertTo(pt));
    double const dist = center.SquaredLength(pt);
    if (dist < minDistance)
    {
      minDistance = dist;
      nearestPt = pt;
    }
  }

  if (!top)
    return false;

  m2::RectD const local = viewport.GetLocalRect();
  double const farDistance =
      std::max(kFarDistanceMeters,
               kFarViewportFactor * mercator::DistanceOnEarth(center, viewport.ConvertFrom(local.RightTop())));
  if (mercator::DistanceOnEarth(center, nearestPt) <= farDistance)
  {
    // Zoom out symmetrically around the center, keeping the original extents.
    m2::PointD const offset = viewport.ConvertTo(nearestPt) - local.Center();
    viewport.Inflate(std::max(0.0, std::fabs(offset.x) - local.SizeX() / 2),
                     std::max(0.0, std::fabs(offset.y) - local.SizeY() / 2));
  }
  else
  {
    double const localizedSize = GetSizeForScale(kLocalizedScale);
    // Too far and spread to be shown at once: show the top result only, like a tap on it.
    if (bounds.SizeX() > localizedSize || bounds.SizeY() > localizedSize)
    {
      count = 1;
      bounds = m2::RectD(viewport.ConvertTo(topPt), 0.0 /* dx */, 0.0 /* dy */);
    }
    // Show all the results, but not closer than the comfort scale (or the single result's own scale).
    double const minSize = GetSizeForScale(count == 1 ? GetScaleForResult(*top) : scales::GetUpperComfortScale());
    bounds.Inflate(std::max(0.0, (minSize - bounds.SizeX()) / 2), std::max(0.0, (minSize - bounds.SizeY()) / 2));
    viewport = m2::AnyRectD(viewport.GlobalZero(), viewport.Angle(), bounds);
  }

  viewport.Inflate(viewport.GetLocalRect().SizeX() * kMarginFactor, viewport.GetLocalRect().SizeY() * kMarginFactor);
  return true;
}
}  // namespace search
