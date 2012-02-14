#include "search_engine.hpp"
#include "result.hpp"
#include "search_query.hpp"

#include "../storage/country_info.hpp"

#include "../indexer/categories_holder.hpp"
#include "../indexer/search_string_utils.hpp"
#include "../indexer/mercator.hpp"

#include "../platform/platform.hpp"

#include "../geometry/distance_on_sphere.hpp"

#include "../base/logging.hpp"
#include "../base/stl_add.hpp"

#include "../std/map.hpp"
#include "../std/vector.hpp"
#include "../std/bind.hpp"


namespace search
{

typedef vector<pair<strings::UniString, uint8_t> > SuggestsContainerT;

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
  map<strings::UniString, uint8_t> m_suggests;

public:
  void operator() (CategoriesHolder::Category::Name const & name)
  {
    strings::UniString const uniName = NormalizeAndSimplifyString(name.m_name);

    uint8_t & score = m_suggests[uniName];
    if (score == 0 || score > name.m_prefixLengthToSuggest)
      score = name.m_prefixLengthToSuggest;
  }

  void GetSuggests(SuggestsContainerT & cont) const
  {
    cont.assign(m_suggests.begin(), m_suggests.end());
  }
};

}


Engine::Engine(IndexType const * pIndex, Reader * pCategoriesR,
               ModelReaderPtr polyR, ModelReaderPtr countryR,
               string const & lang)
  : m_pIndex(pIndex), m_pData(new EngineData(pCategoriesR, polyR, countryR))
{
  InitSuggestions doInit;
  m_pData->m_categories.ForEachName(bind<void>(ref(doInit), _1));
  doInit.GetSuggests(m_pData->m_stringsToSuggest);

  m_pQuery.reset(new Query(pIndex,
                           &m_pData->m_categories,
                           &m_pData->m_stringsToSuggest,
                           &m_pData->m_infoGetter));
  m_pQuery->SetPreferredLanguage(lang);
}

Engine::~Engine()
{
}

namespace
{
  m2::PointD GetViewportXY(double lat, double lon)
  {
    return m2::PointD(MercatorBounds::LonToX(lon), MercatorBounds::LatToY(lat));
  }
  m2::RectD GetViewportRect(double lat, double lon, double radius = 20000)
  {
    return MercatorBounds::MetresToXY(lon, lat, radius);
  }
}

void Engine::PrepareSearch(m2::RectD const & viewport, bool nearMe,
                           double lat, double lon)
{
  // bind does copy of viewport
  m2::RectD const r = (nearMe ? GetViewportRect(lat, lon) : viewport);
  GetPlatform().RunAsync(bind(&Engine::SetViewportAsync, this, r));
}

void Engine::Search(SearchParams const & params, m2::RectD const & viewport)
{
  Platform & p = GetPlatform();

  {
    threads::MutexGuard guard(m_updateMutex);

    m_params = params;
    m_viewport = viewport;
  }

  p.RunAsync(bind(&Engine::SearchAsync, this));
}

void Engine::SetViewportAsync(m2::RectD const & viewport)
{
  // First of all - cancel previous query.
  m_pQuery->DoCancel();

  // Enter to run new search.
  threads::MutexGuard searchGuard(m_searchMutex);

  m_pQuery->SetViewport(viewport);
}

void Engine::SearchAsync()
{
  // First of all - cancel previous query.
  m_pQuery->DoCancel();

  // Enter to run new search.
  threads::MutexGuard searchGuard(m_searchMutex);

  // Get current search params.
  SearchParams params;
  m2::RectD viewport;

  {
    threads::MutexGuard updateGuard(m_updateMutex);
    params = m_params;
    viewport = m_viewport;
  }

  // Initialize query.
  bool const nearMe = params.IsNearMeMode();
  if (nearMe)
    m_pQuery->SetViewport(GetViewportRect(params.m_lat, params.m_lon));
  else
    m_pQuery->SetViewport(viewport);

  if (params.m_validPos)
    m_pQuery->SetPosition(GetViewportXY(params.m_lat, params.m_lon));
  else
    m_pQuery->NullPosition();

  unsigned int const resultsNeeded = 10;
  Results res;

  // Run first search with needed params.
  try
  {
    m_pQuery->Search(params.m_query, res, resultsNeeded);
  }
  catch (Query::CancelException const &)
  {
  }

  // If not enough results, run second search with "Near Me" viewport.
  if (!m_pQuery->IsCanceled() && !nearMe && params.m_validPos && (res.Count() < resultsNeeded))
  {
    m_pQuery->SetViewport(GetViewportRect(params.m_lat, params.m_lon));

    try
    {
      m_pQuery->Search(params.m_query, res, resultsNeeded);
    }
    catch (Query::CancelException const &)
    {
    }
  }

  // Emit results in any way, even if search was canceled.
  params.m_callback(res);
}

string Engine::GetCountryFile(m2::PointD const & pt) const
{
  return m_pData->m_infoGetter.GetRegionFile(pt);
}

}  // namespace search
