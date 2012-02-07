#include "search_engine.hpp"
#include "category_info.hpp"
#include "result.hpp"
#include "search_query.hpp"

#include "../storage/country_info.hpp"

#include "../indexer/categories_holder.hpp"
#include "../indexer/search_delimiters.hpp"
#include "../indexer/search_string_utils.hpp"
#include "../indexer/mercator.hpp"

#include "../platform/platform.hpp"

#include "../geometry/distance_on_sphere.hpp"

#include "../base/logging.hpp"
#include "../base/stl_add.hpp"

#include "../std/algorithm.hpp"
#include "../std/function.hpp"
#include "../std/map.hpp"
#include "../std/string.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"
#include "../std/bind.hpp"


namespace search
{

class EngineData
{
public:
  EngineData(ModelReaderPtr polyR, ModelReaderPtr countryR)
    : m_infoGetter(polyR, countryR) {}

  multimap<strings::UniString, uint32_t> m_categories;
  vector<pair<strings::UniString, uint8_t> > m_stringsToSuggest;
  storage::CountryInfoGetter m_infoGetter;
};

Engine::Engine(IndexType const * pIndex, CategoriesHolder * pCategories,
               ModelReaderPtr polyR, ModelReaderPtr countryR,
               string const & lang)
  : m_pIndex(pIndex), m_pData(new EngineData(polyR, countryR))
{
  if (pCategories)
  {
    InitializeCategoriesAndSuggestStrings(*pCategories);
    delete pCategories;
  }

  m_pQuery.reset(new Query(pIndex,
                           &m_pData->m_categories,
                           &m_pData->m_stringsToSuggest,
                           &m_pData->m_infoGetter));
  m_pQuery->SetPreferredLanguage(lang);
}

Engine::~Engine()
{
}

void Engine::InitializeCategoriesAndSuggestStrings(CategoriesHolder const & categories)
{
  m_pData->m_categories.clear();
  m_pData->m_stringsToSuggest.clear();

  map<strings::UniString, uint8_t> stringsToSuggest;
  for (CategoriesHolder::const_iterator it = categories.begin(); it != categories.end(); ++it)
  {
    for (size_t i = 0; i < it->m_synonyms.size(); ++i)
    {
      CategoriesHolder::Category::Name const & name = it->m_synonyms[i];
      strings::UniString const uniName = NormalizeAndSimplifyString(name.m_name);

      uint8_t & score = stringsToSuggest[uniName];
      if (score == 0 || score > name.m_prefixLengthToSuggest)
        score = name.m_prefixLengthToSuggest;

      vector<strings::UniString> tokens;
      SplitUniString(uniName, MakeBackInsertFunctor(tokens), CategoryDelimiters());
      for (size_t j = 0; j < tokens.size(); ++j)
        for (size_t k = 0; k < it->m_types.size(); ++k)
          m_pData->m_categories.insert(make_pair(tokens[j], it->m_types[k]));
    }
  }

  m_pData->m_stringsToSuggest.assign(stringsToSuggest.begin(), stringsToSuggest.end());
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

void Engine::Search(SearchParams const & params)
{
  {
    threads::MutexGuard guard(m_updateMutex);
    m_params = params;
  }

  GetPlatform().RunAsync(bind(&Engine::SearchAsync, this));
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
  {
    threads::MutexGuard updateGuard(m_updateMutex);
    params = m_params;
  }

  // Initialize query.
  if (params.IsNearMeMode())
    m_pQuery->SetViewport(GetViewportRect(params.m_lat, params.m_lon));

  if (params.m_validPos)
    m_pQuery->SetPosition(GetViewportXY(params.m_lat, params.m_lon));
  else
    m_pQuery->NullPosition();

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
