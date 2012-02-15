#include "search_engine.hpp"
#include "result.hpp"
#include "search_query.hpp"

#include "../storage/country_info.hpp"

#include "../indexer/categories_holder.hpp"
#include "../indexer/search_string_utils.hpp"
#include "../indexer/mercator.hpp"
#include "../indexer/scales.hpp"

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

  /// Check rects for optimal search (avoid duplicating).
  void AnalizeRects(m2::RectD arrRects[2])
  {
    if (arrRects[1].IsRectInside(arrRects[0]))
      arrRects[0].MakeEmpty();
    else
    {
      if (arrRects[0].IsRectInside(arrRects[1]) &&
          scales::GetScaleLevel(arrRects[0]) + Query::m_scaleDepthSearch >= scales::GetUpperScale())
      {
        arrRects[1].MakeEmpty();
      }
    }
  }
}

void Engine::PrepareSearch(m2::RectD const & viewport,
                           bool hasPt, double lat, double lon)
{
  m2::RectD const nearby = (hasPt ? GetViewportRect(lat, lon) : m2::RectD());

  // bind does copy of all rects
  GetPlatform().RunAsync(bind(&Engine::SetViewportAsync, this, viewport, nearby));
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

void Engine::SetViewportAsync(m2::RectD const & viewport, m2::RectD const & nearby)
{
  // First of all - cancel previous query.
  m_pQuery->DoCancel();

  // Enter to run new search.
  threads::MutexGuard searchGuard(m_searchMutex);

  m2::RectD arrRects[] = { viewport, nearby };
  AnalizeRects(arrRects);

  m_pQuery->SetViewport(arrRects, ARRAY_SIZE(arrRects));
}

void Engine::SearchAsync()
{
  // First of all - cancel previous query.
  m_pQuery->DoCancel();

  // Enter to run new search.
  threads::MutexGuard searchGuard(m_searchMutex);

  // Get current search params.
  SearchParams params;
  m2::RectD arrRects[2];

  {
    threads::MutexGuard updateGuard(m_updateMutex);
    params = m_params;
    arrRects[0] = m_viewport;
  }

  // Initialize query.
  bool worldSearch = true;
  if (params.m_validPos)
  {
    m_pQuery->SetPosition(GetViewportXY(params.m_lat, params.m_lon));

    arrRects[1] = GetViewportRect(params.m_lat, params.m_lon);

    // Do not search in viewport for "NearMe" mode.
    if (params.IsNearMeMode())
    {
      worldSearch = false;
      arrRects[0].MakeEmpty();
    }
    else
      AnalizeRects(arrRects);
  }
  else
    m_pQuery->NullPosition();

  m_pQuery->SetViewport(arrRects, 2);
  m_pQuery->SetSearchInWorld(worldSearch);

  Results res;

  try
  {
    m_pQuery->Search(params.m_query, res);
  }
  catch (Query::CancelException const &)
  {
  }

  // Emit results in any way, even if search was canceled.
  params.m_callback(res);
}

string Engine::GetCountryFile(m2::PointD const & pt) const
{
  return m_pData->m_infoGetter.GetRegionFile(pt);
}

}  // namespace search
