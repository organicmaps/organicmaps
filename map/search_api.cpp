#include "map/search_api.hpp"

#include "map/everywhere_search_params.hpp"
#include "map/viewport_search_params.hpp"

#include "search/geometry_utils.hpp"
#include "search/hotels_filter.hpp"

#include "storage/downloader_search_params.hpp"

#include "platform/preferred_languages.hpp"

#include "geometry/mercator.hpp"

#include "base/string_utils.hpp"

#include <string>
#include <vector>

using namespace search;
using namespace std;

namespace
{
double const kDistEqualQueryMeters = 100.0;

// Cancels search query by |handle|.
void CancelQuery(weak_ptr<ProcessorHandle> & handle)
{
  if (auto queryHandle = handle.lock())
    queryHandle->Cancel();
  handle.reset();
}

bool IsCianMode(string query)
{
  strings::Trim(query);
  strings::AsciiToLower(query);
  return query == "cian";
}
}  // namespace

SearchAPI::SearchAPI(Index & index, storage::Storage const & storage,
                     storage::CountryInfoGetter const & infoGetter, Delegate & delegate)
  : m_index(index)
  , m_storage(storage)
  , m_infoGetter(infoGetter)
  , m_delegate(delegate)
  , m_engine(m_index, GetDefaultCategories(), m_infoGetter,
             Engine::Params(languages::GetCurrentOrig() /* locale */, 1 /* params.m_numThreads */))
{
}

void SearchAPI::OnViewportChanged(m2::RectD const & viewport)
{
  m_viewport = viewport;

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
}

bool SearchAPI::SearchEverywhere(EverywhereSearchParams const & params)
{
  UpdateSponsoredMode(params.m_query, params.m_bookingFilterParams);

  SearchParams p;
  p.m_query = params.m_query;
  p.m_inputLocale = params.m_inputLocale;
  p.m_mode = Mode::Everywhere;
  p.m_position = m_delegate.GetCurrentPosition();
  SetViewportIfPossible(p);  // Search request will be delayed if viewport is not available.
  p.m_maxNumResults = SearchParams::kDefaultNumResultsEverywhere;
  p.m_suggestsEnabled = true;
  p.m_needAddress = true;
  p.m_needHighlighting = true;
  p.m_hotelsFilter = params.m_hotelsFilter;
  p.m_cianMode = m_sponsoredMode == SponsoredMode::Cian;

  p.m_onResults = EverywhereSearchCallback(
      static_cast<EverywhereSearchCallback::Delegate &>(*this),
      [this, params](Results const & results, vector<bool> const & isLocalAdsCustomer) {
        if (params.m_onResults)
          RunUITask([params, results, isLocalAdsCustomer] {
            params.m_onResults(results, isLocalAdsCustomer);
          });
        if (results.IsEndedNormal() && !params.m_bookingFilterParams.IsEmpty())
        {
          m_delegate.FilterSearchResultsOnBooking(params.m_bookingFilterParams, results,
                                                  false /* inViewport */);
        }
      });

  return Search(p, true /* forceSearch */);
}

bool SearchAPI::SearchInViewport(ViewportSearchParams const & params)
{
  UpdateSponsoredMode(params.m_query, params.m_bookingFilterParams);

  SearchParams p;
  p.m_query = params.m_query;
  p.m_inputLocale = params.m_inputLocale;
  p.m_position = m_delegate.GetCurrentPosition();
  SetViewportIfPossible(p);  // Search request will be delayed if viewport is not available.
  p.m_maxNumResults = SearchParams::kDefaultNumResultsInViewport;
  p.m_mode = Mode::Viewport;
  p.m_suggestsEnabled = false;
  p.m_needAddress = false;
  p.m_needHighlighting = false;
  p.m_hotelsFilter = params.m_hotelsFilter;
  p.m_cianMode = m_sponsoredMode == SponsoredMode::Cian;

  p.m_onStarted = [this, params] {
    if (params.m_onStarted)
      RunUITask([params]() { params.m_onStarted(); });
  };

  p.m_onResults = ViewportSearchCallback(
      static_cast<ViewportSearchCallback::Delegate &>(*this),
      [this, params](Results const & results) {
        if (results.IsEndMarker() && params.m_onCompleted)
          RunUITask([params, results] { params.m_onCompleted(results); });
        if (results.IsEndedNormal() && !params.m_bookingFilterParams.IsEmpty())
        {
          m_delegate.FilterSearchResultsOnBooking(params.m_bookingFilterParams, results,
                                                  true /* inViewport */);
        }
      });

  if (m_sponsoredMode == SponsoredMode::Booking)
    m_delegate.OnBookingFilterParamsUpdate(params.m_bookingFilterParams.m_params);

  return Search(p, false /* forceSearch */);
}

bool SearchAPI::SearchInDownloader(storage::DownloaderSearchParams const & params)
{
  m_sponsoredMode = SponsoredMode::None;

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

  p.m_onResults = DownloaderSearchCallback(static_cast<DownloaderSearchCallback::Delegate &>(*this),
                                           m_index, m_infoGetter, m_storage, params);

  return Search(p, true /* forceSearch */);
}

void SearchAPI::PokeSearchInViewport(bool forceSearch)
{
  if (!m_isViewportInitialized || !IsViewportSearchActive())
    return;

  // Copy is intentional here, to skip possible duplicating requests.
  auto params = m_searchIntents[static_cast<size_t>(Mode::Viewport)].m_params;
  SetViewportIfPossible(params);
  params.m_position = m_delegate.GetCurrentPosition();
  Search(params, forceSearch);
}

void SearchAPI::CancelSearch(Mode mode)
{
  ASSERT_NOT_EQUAL(mode, Mode::Count, ());

  if (mode == Mode::Viewport)
  {
    m_sponsoredMode = SponsoredMode::None;

    m_delegate.ClearViewportSearchResults();
    m_delegate.SetSearchDisplacementModeEnabled(false /* enabled */);
  }

  auto & intent = m_searchIntents[static_cast<size_t>(mode)];
  intent.m_params.Clear();
  CancelQuery(intent.m_handle);
}

void SearchAPI::CancelAllSearches()
{
  for (size_t i = 0; i < static_cast<size_t>(Mode::Count); ++i)
    CancelSearch(static_cast<Mode>(i));
}

void SearchAPI::RunUITask(std::function<void()> fn) { return m_delegate.RunUITask(fn); }

void SearchAPI::SetHotelDisplacementMode()
{
  return m_delegate.SetSearchDisplacementModeEnabled(true);
}

bool SearchAPI::IsViewportSearchActive() const
{
  return !m_searchIntents[static_cast<size_t>(Mode::Viewport)].m_params.m_query.empty();
}

void SearchAPI::ShowViewportSearchResults(bool clear, search::Results::ConstIter begin,
                                          search::Results::ConstIter end)
{
  return m_delegate.ShowViewportSearchResults(clear, begin, end);
}

bool SearchAPI::IsLocalAdsCustomer(Result const & result) const
{
  return m_delegate.IsLocalAdsCustomer(result);
}

bool SearchAPI::Search(SearchParams const & params, bool forceSearch)
{
  if (m_delegate.ParseSearchQueryCommand(params))
    return false;

  auto const mode = params.m_mode;
  auto & intent = m_searchIntents[static_cast<size_t>(mode)];

  if (!forceSearch && QueryMayBeSkipped(intent.m_params, params))
    return false;

  intent.m_params = params;

  // Cancels previous search request (if any) and initiates a new
  // search request.
  CancelQuery(intent.m_handle);

  intent.m_params.m_minDistanceOnMapBetweenResults = m_delegate.GetMinDistanceBetweenResults();

  Search(intent);

  return true;
}

void SearchAPI::Search(SearchIntent & intent)
{
  if (!m_isViewportInitialized)
  {
    intent.m_isDelayed = true;
    return;
  }

  intent.m_handle = m_engine.Search(intent.m_params);
  intent.m_isDelayed = false;
}

void SearchAPI::SetViewportIfPossible(SearchParams & params)
{
  if (m_isViewportInitialized)
    params.m_viewport = m_viewport;
}

bool SearchAPI::QueryMayBeSkipped(SearchParams const & prevParams,
                                  SearchParams const & currParams) const
{
  auto const & prevViewport = prevParams.m_viewport;
  auto const & currViewport = currParams.m_viewport;

  if (!prevParams.IsEqualCommon(currParams))
    return false;

  if (!prevViewport.IsValid() ||
      !IsEqualMercator(prevViewport, currViewport, kDistEqualQueryMeters))
  {
    return false;
  }

  if (prevParams.m_position && currParams.m_position &&
      MercatorBounds::DistanceOnEarth(*prevParams.m_position, *currParams.m_position) >
          kDistEqualQueryMeters)
  {
    return false;
  }

  if (static_cast<bool>(prevParams.m_position) != static_cast<bool>(currParams.m_position))
    return false;

  if (!hotels_filter::Rule::IsIdentical(prevParams.m_hotelsFilter, currParams.m_hotelsFilter))
    return false;

  return true;
}

void SearchAPI::UpdateSponsoredMode(std::string const & query,
                                    booking::filter::availability::Params const & params)
{
  m_sponsoredMode = SponsoredMode::None;
  // TODO: delete me after Cian project is finished.
  if (IsCianMode(query))
    m_sponsoredMode = SponsoredMode::Cian;
  if (!params.IsEmpty())
    m_sponsoredMode = SponsoredMode::Booking;
}

string DebugPrint(SearchAPI::SponsoredMode mode)
{
  switch (mode)
  {
  case SearchAPI::SponsoredMode::None: return "None";
  case SearchAPI::SponsoredMode::Cian: return "Cian";
  case SearchAPI::SponsoredMode::Booking: return "Booking";
  }
}
