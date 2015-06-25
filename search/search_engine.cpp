#include "search_engine.hpp"
#include "search_query.hpp"
#include "geometry_utils.hpp"

#include "storage/country_info.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/search_string_utils.hpp"
#include "indexer/mercator.hpp"
#include "indexer/scales.hpp"
#include "indexer/classificator.hpp"

#include "platform/platform.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "base/stl_add.hpp"

#include "std/map.hpp"
#include "std/vector.hpp"
#include "std/bind.hpp"

#include "3party/Alohalytics/src/alohalytics.h"


namespace search
{

double const DIST_EQUAL_QUERY = 100.0;

typedef vector<Query::SuggestT> SuggestsContainerT;

class EngineData
{
public:
  EngineData(Reader * pCategoriesR, ModelReaderPtr polyR, ModelReaderPtr countryR)
    : m_categories(pCategoriesR), m_infoGetter(polyR, countryR)
  {
  }

  CategoriesHolder m_categories;
  SuggestsContainerT m_stringsToSuggest;
  storage::CountryInfoGetter m_infoGetter;
};

namespace
{

class InitSuggestions
{
  typedef map<pair<strings::UniString, int8_t>, uint8_t> SuggestMapT;
  SuggestMapT m_suggests;

public:
  void operator() (CategoriesHolder::Category::Name const & name)
  {
    if (name.m_prefixLengthToSuggest != CategoriesHolder::Category::EMPTY_PREFIX_LENGTH)
    {
      strings::UniString const uniName = NormalizeAndSimplifyString(name.m_name);

      uint8_t & score = m_suggests[make_pair(uniName, name.m_locale)];
      if (score == 0 || score > name.m_prefixLengthToSuggest)
        score = name.m_prefixLengthToSuggest;
    }
  }

  void GetSuggests(SuggestsContainerT & cont) const
  {
    cont.reserve(m_suggests.size());
    for (SuggestMapT::const_iterator i = m_suggests.begin(); i != m_suggests.end(); ++i)
      cont.push_back(Query::SuggestT(i->first.first, i->second, i->first.second));
  }
};

}


Engine::Engine(IndexType const * pIndex, Reader * pCategoriesR,
               ModelReaderPtr polyR, ModelReaderPtr countryR,
               string const & locale)
  : m_pData(new EngineData(pCategoriesR, polyR, countryR))
{
  m_isReadyThread.clear();

  InitSuggestions doInit;
  m_pData->m_categories.ForEachName(bind<void>(ref(doInit), _1));
  doInit.GetSuggests(m_pData->m_stringsToSuggest);

  m_pQuery.reset(new Query(pIndex,
                           &m_pData->m_categories,
                           &m_pData->m_stringsToSuggest,
                           &m_pData->m_infoGetter));
  m_pQuery->SetPreferredLocale(locale);
}

Engine::~Engine()
{
}

void Engine::SupportOldFormat(bool b)
{
  m_pQuery->SupportOldFormat(b);
}

void Engine::PrepareSearch(m2::RectD const & viewport)
{
  // bind does copy of all rects
  GetPlatform().RunAsync(bind(&Engine::SetViewportAsync, this, viewport));
}

bool Engine::Search(SearchParams const & params, m2::RectD const & viewport)
{
  // Check for equal query.
  // There is no need to synchronize here for reading m_params,
  // because this function is always called from main thread (one-by-one for queries).

  if (!params.IsForceSearch() &&
      m_params.IsEqualCommon(params) &&
      (m_viewport.IsValid() && IsEqualMercator(m_viewport, viewport, DIST_EQUAL_QUERY)))
  {
    if (m_params.IsSearchAroundPosition() &&
        ms::DistanceOnEarth(m_params.m_lat, m_params.m_lon, params.m_lat, params.m_lon) > DIST_EQUAL_QUERY)
    {
      // Go forward only if we search around position and it's changed significantly.
    }
    else
    {
      // Skip this query in all other cases.
      return false;
    }
  }

  {
    // Assign new search params.
    // Put the synch here, because this params are reading in search threads.
    threads::MutexGuard guard(m_updateMutex);
    m_params = params;
    m_viewport = viewport;
  }

  // Run task.
  GetPlatform().RunAsync(bind(&Engine::SearchAsync, this));

  return true;
}

void Engine::GetResults(Results & res)
{
  threads::MutexGuard guard(m_searchMutex);
  res = m_searchResults;
}

void Engine::SetViewportAsync(m2::RectD const & viewport)
{
  // First of all - cancel previous query.
  m_pQuery->Cancel();

  // Enter to run new search.
  threads::MutexGuard searchGuard(m_searchMutex);

  m2::RectD r(viewport);
  (void)GetInflatedViewport(r);
  m_pQuery->SetViewport(r, true);
}

void Engine::EmitResults(SearchParams const & params, Results & res)
{
  m_searchResults = res;

  // Basic test of our statistics engine.
  alohalytics::LogEvent("searchEmitResults",
                        alohalytics::TStringMap({{params.m_query, strings::to_string(res.GetCount())}}));

  params.m_callback(res);
}

void Engine::SetRankPivot(SearchParams const & params,
                          m2::RectD const & viewport, bool viewportSearch)
{
  if (!viewportSearch && params.IsValidPosition())
  {
    m2::PointD const pos = MercatorBounds::FromLatLon(params.m_lat, params.m_lon);
    if (m2::Inflate(viewport, viewport.SizeX() / 4.0, viewport.SizeY() / 4.0).IsPointInside(pos))
    {
      m_pQuery->SetRankPivot(pos);
      return;
    }
  }

  m_pQuery->SetRankPivot(viewport.Center());
}

void Engine::SearchAsync()
{
  if (m_isReadyThread.test_and_set())
    return;

  // First of all - cancel previous query.
  m_pQuery->Cancel();

  // Enter to run new search.
  threads::MutexGuard searchGuard(m_searchMutex);

  m_isReadyThread.clear();

  // Get current search params.
  SearchParams params;
  m2::RectD viewport;
  bool oneTimeSearch = false;

  {
    threads::MutexGuard updateGuard(m_updateMutex);
    params = m_params;

    if (params.GetSearchRect(viewport))
      oneTimeSearch = true;
    else
      viewport = m_viewport;
  }

  bool const viewportSearch = params.HasSearchMode(SearchParams::IN_VIEWPORT_ONLY);

  // Initialize query.
  m_pQuery->Init(viewportSearch);

  SetRankPivot(params, viewport, viewportSearch);

  m_pQuery->SetSearchInWorld(params.HasSearchMode(SearchParams::SEARCH_WORLD));

  // Language validity is checked inside
  m_pQuery->SetInputLocale(params.m_inputLocale);

  ASSERT(!params.m_query.empty(), ());
  m_pQuery->SetQuery(params.m_query);

  Results res;

  // Call m_pQuery->IsCanceled() everywhere it needed without storing return value.
  // This flag can be changed from another thread.

  m_pQuery->SearchCoordinates(params.m_query, res);

  try
  {
    // Do search for address in all modes.
    // params.HasSearchMode(SearchParams::SEARCH_ADDRESS)

    if (viewportSearch)
    {
      m_pQuery->SetViewport(viewport, true);
      m_pQuery->SearchViewportPoints(res);

      if (res.GetCount() > 0)
        EmitResults(params, res);
    }
    else
    {
      while (!m_pQuery->IsCancelled())
      {
        bool const isInflated = GetInflatedViewport(viewport);
        size_t const oldCount = res.GetCount();

        m_pQuery->SetViewport(viewport, oneTimeSearch);
        m_pQuery->Search(res, RESULTS_COUNT);

        size_t const newCount = res.GetCount();
        bool const exit = (oneTimeSearch || !isInflated || newCount >= RESULTS_COUNT);

        if (exit || oldCount != newCount)
          EmitResults(params, res);

        if (exit)
          break;
      }
    }
  }
  catch (Query::CancelException const &)
  {
  }

  // Make additional search in whole mwm when not enough results (only for non-empty query).
  size_t const count = res.GetCount();
  if (!viewportSearch && !m_pQuery->IsCancelled() && count < RESULTS_COUNT)
  {
    try
    {
      m_pQuery->SearchAdditional(res, RESULTS_COUNT);
    }
    catch (Query::CancelException const &)
    {
    }

    // Emit if we have more results.
    if (res.GetCount() > count)
      EmitResults(params, res);
  }

  // Emit finish marker to client.
  params.m_callback(Results::GetEndMarker(m_pQuery->IsCancelled()));
}

string Engine::GetCountryFile(m2::PointD const & pt)
{
  threads::MutexGuard searchGuard(m_searchMutex);

  return m_pData->m_infoGetter.GetRegionFile(pt);
}

string Engine::GetCountryCode(m2::PointD const & pt)
{
  threads::MutexGuard searchGuard(m_searchMutex);

  storage::CountryInfo info;
  m_pData->m_infoGetter.GetRegionInfo(pt, info);
  return info.m_flag;
}

template <class T> string Engine::GetCountryNameT(T const & t)
{
  threads::MutexGuard searchGuard(m_searchMutex);

  storage::CountryInfo info;
  m_pData->m_infoGetter.GetRegionInfo(t, info);
  return info.m_name;
}

string Engine::GetCountryName(m2::PointD const & pt)
{
  return GetCountryNameT(pt);
}

string Engine::GetCountryName(string const & id)
{
  return GetCountryNameT(id);
}

bool Engine::GetNameByType(uint32_t type, int8_t locale, string & name) const
{
  uint8_t level = ftype::GetLevel(type);
  ASSERT_GREATER(level, 0, ());

  while (true)
  {
    if (m_pData->m_categories.GetNameByType(type, locale, name))
      return true;

    if (--level == 0)
      break;

    ftype::TruncValue(type, level);
  }

  return false;
}

m2::RectD Engine::GetCountryBounds(string const & file) const
{
  return m_pData->m_infoGetter.CalcLimitRect(file);
}

void Engine::ClearViewportsCache()
{
  threads::MutexGuard guard(m_searchMutex);

  m_pQuery->ClearCaches();
}

void Engine::ClearAllCaches()
{
  //threads::MutexGuard guard(m_searchMutex);

  // Trying to lock mutex, because this function calls on memory warning notification.
  // So that allows to prevent lock of UI while search query wouldn't be processed.
  if (m_searchMutex.TryLock())
  {
    m_pQuery->ClearCaches();
    m_pData->m_infoGetter.ClearCaches();

    m_searchMutex.Unlock();
  }
}

}  // namespace search
